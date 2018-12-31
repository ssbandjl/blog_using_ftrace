/*
 * Copyright (C) 2019 IT Dev Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/* TODO: I use strcpy not strncpy, and some other variables - I think they are
   safe in the context in which they are being used but this warrents a
   check as they are standards stack smask weak points.*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <error.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <linux/limits.h>

#include "itd_ftrace_debugging.h"
#include "cstrings/get_line/get_line.h"

#define TRACE_BUFFER_SIZE 256U

/** @brief Buffer to print trace_print() output to before sending to ftrace */
static char trace_buffer[TRACE_BUFFER_SIZE];

/** @brief Absolute path to tracefs "tracing_on" file */
static char *tracing_on_file_path = NULL;

/** @brief Absolute path to tracefs "trace_marker" file */
static char *trace_marker_file_path = NULL;

/** @brief file handle for tracefs "tracing_on" file */
static int tracing_toggle_fh = -1;

/** @brief file handle for tracefs "trace_marker" file */
static int trace_marker_fh = -1;

/** @brief caches state of tracefs "tracing_on" file value */
static bool trace_is_enabled = 0;

/**
 * @brief Allocate and set buffers for absolute paths to tracefs files
 *        "tracing_on" and "trace_marker"
 *
 * Depending on the version of Linux, the filesystem may be debugfs or tracefs
 * If its tracefs then just take the path given, if its debugfs then we have to
 * add a subdirectory "tracing" to the path as the ftrace files are located
 * in this subdirectory of the debugfs.
 *
 * @param mount_path Absolute path at which the debugfs/tracefs is mounted
 * @param is_tracefs True when mount_path refers to a tracefs mount, false
 *                   otherwise.
 *
 * @return 0 on success or -ENOMEM if buffer allocation fails.
 *
 * @post The global pointers tracing_on_file_path and trace_marker_file_path
 *       will point to malloced string buffers on success.
 */
static int allocate_and_set_tracefs_file_paths(const char *const mount_path,
                                               const bool is_tracefs)
{
    static char **tracefs_file_paths[] = {
        &tracing_on_file_path, &trace_marker_file_path};
    static const char *const trace_fs_file_names[] = {
        "/tracing_on", "/trace_marker"};
    static const size_t trace_fs_file_names_strlen[] = {11U, 13U};
    /* Debugfs files must include an extra subdir - make sure there's room */
    const size_t debufs_path_extention_strlen = is_tracefs ? 0U : 8U;
    const size_t mount_path_strlen = strnlen(mount_path, PATH_MAX);
    unsigned int i = 0U;

    /* For each variable "tracing_on_file_path", "trace_marker_file_path": */
    for (; i < 2U; ++i) {
        size_t buffer_offset = 0U;

        *tracefs_file_paths[i] = malloc(
            mount_path_strlen + debufs_path_extention_strlen + 
            trace_fs_file_names_strlen[i] + 1U); /* +1 for '\0' */
        if (!tracing_on_file_path)
            goto out_of_memory;

        strcpy(*tracefs_file_paths[i], mount_path);
        buffer_offset += mount_path_strlen;
        if (!is_tracefs) {
            strcpy(*tracefs_file_paths[i] + buffer_offset, "/tracing");
            buffer_offset += 8U;
        }
        strcpy(*tracefs_file_paths[i] + buffer_offset, trace_fs_file_names[i]);
        buffer_offset += trace_fs_file_names_strlen[i];
        (*tracefs_file_paths[i])[buffer_offset] = '\0';
    }

    return 0;

out_of_memory:
    for (i = 0U; i < 2U; ++i) {
        free(*tracefs_file_paths[i]);
        *tracefs_file_paths[i] = NULL;
    }
    return -ENOMEM;
}

/**
 * @brief Parse one line from /proc/mounts to determine if it specifies the
 *        mount point for either tracefs or debugfs.
 *
 * @return 0 on success, -ENOENT if the line does not contain a tracefs/debugfs
 *         mount point or -ENOMEM if an out-of-memory error occurred.
 *
 * @param line Pointer to null terminated string representing a line from
 *             /proc/mountfs.
 * @param is_tracefs True if the tracefs filesystem should be searched for,
 *                   false if the debugfs filesystem should be searched for.
 *
 * @post The global pointers tracing_on_file_path and trace_marker_file_path 
 *       will point to malloced string buffers on success.
 * @post The contents of the line buffer will be modified.
 */
static int parse_proc_mounts_line(char *const line, const bool is_tracefs)
{
    char *saveptr = NULL;
    char *path_token = NULL;
    char *token;

    /* Ignore first token */
    token = strtok_r(line, " ", &saveptr);
    if (!token)
        return -ENOENT;

    /* Second token is the mount path */
    token = strtok_r(NULL, " ", &saveptr);
    if (!token)
        return -ENOENT;
    path_token = token;

    /* Third token is the filesystem type */
    token = strtok_r(NULL, " ", &saveptr);
    if (!token)
        return -ENOENT;

    if (strcmp(token, (is_tracefs ? "tracefs" : "debugfs")) == 0) {
        return allocate_and_set_tracefs_file_paths(path_token, is_tracefs);
    }

    return -ENOENT;
}

/*
 * @brief Parse /proc/mounts to find out where the ftrace files reside.
 *
 * Because the file system is mounted this can be anywhere, although a standard
 * location is likely. The files can either be part of debugfs pre Linux 4.1
 * or part of tracefs.
 *
 * Search for tracefs first as this is what it is called in Linux 4.1 onwards.
 * Search for debugfs second, as it is parts of the debugfs pre Linux 4.1. We
 * must check in this order as it is possible for both tracefs and debugfs to
 * exist.
 *
 * @return 0 on success, -ENOENT a tracefs/debugfs mount was not found or
 *         -ENOMEM if an out-of-memory error occurred.
 *
 * @post The global pointers tracing_on_file_path and trace_marker_file_path
 *       will point to malloced string buffers on success.
 */
static int find_tracefs(void)
{
    int result;
    char *line = NULL;
    size_t buff_size = 0;
    unsigned int i = 0;
    FILE *mounts_fh;

    /* If we've done this already, don't do it again! */
    if (tracing_on_file_path)
        return 0;

    mounts_fh = fopen("/proc/mounts", "r");
    if (!mounts_fh)
        return errno;

    for(; i < 2; ++i) {
        const bool is_tracefs = i == 0;
        while(1) {
            const int read_line_result = read_line(mounts_fh, &line, &buff_size);
            if (read_line_result < 0) {
                result = read_line_result;
                break;
            } else if (line == NULL) {
                result = -ENOENT;
                break;
            }

            result = parse_proc_mounts_line(line, is_tracefs);
            free(line);
            line = NULL;
            if (result != -ENOENT)
                break;
        }

        if (result != -ENOENT)
            break;

        rewind(mounts_fh);
    }

    fclose(mounts_fh);
    return result;
}

int itd_init_debug_tracing(void)
{
    const int result = find_tracefs();
    if (result)
        goto exit_no_tracefs;

    tracing_toggle_fh = open(tracing_on_file_path, O_WRONLY);
    if (tracing_toggle_fh < 0)
        goto exit_no_toggle;

    trace_marker_fh = open(trace_marker_file_path, O_WRONLY);
    if (trace_marker_fh < 0)
        goto exit_no_marker;

    itd_trace_off();

    return 0;

exit_no_marker:
    close(tracing_toggle_fh);
exit_no_toggle:
    free(tracing_on_file_path);
    free(trace_marker_file_path);
exit_no_tracefs:
    return -1;
}

void itd_trace_on(void)
{
    write(tracing_toggle_fh, "1", 1);
    trace_is_enabled = true;
}

void itd_trace_off(void)
{
    write(tracing_toggle_fh, "0", 1);
    trace_is_enabled = false;
}

void itd_uninit_debug_tracing(void)
{
    close(tracing_toggle_fh);
    close(trace_marker_fh);
    tracing_toggle_fh = -1;
    trace_marker_fh = -1;
    trace_is_enabled = false;
    free(tracing_on_file_path);
    free(trace_marker_file_path);
}

void itd_trace_print(const char *const fmt, ...)
{
    const bool prev_trace_is_enabled = trace_is_enabled;
    va_list ap;

    if (!prev_trace_is_enabled) {
        itd_trace_on();
    }

    va_start(ap, fmt);
    const int count = vsnprintf(trace_buffer, TRACE_BUFFER_SIZE, fmt, ap);
    if (count > 0)
        write(trace_marker_fh, trace_buffer, (size_t)count);
    va_end(ap);

    if (!prev_trace_is_enabled) {
        itd_trace_off();
    }
}
