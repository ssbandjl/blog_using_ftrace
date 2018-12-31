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
#ifndef ITD_FTRACE_DEBUGGING_H
#define ITD_FTRACE_DEBUGGING_H

/**
 * @brief Initialise the ftrace debugging library. Function must only be called
 *        once.
 *
 * Library will allocate resources upon successfull initialisation so
 * itd_uninit_debug_tracing() must be called when you are finished using ftrace,
 * or at least before your program exists.
 *
 * @post the files /sys/kernel/debug/tracing/tracing_on and
 *       /sys/kernel/debug/tracing/trace_marker are open and memory has been
 *       allocated.
 */
int itd_init_debug_tracing(void);

/**
 * @brief Close down the ftrace debugging library and free all allocated
 *        resources. Function must only be called once.
 */
void itd_uninit_debug_tracing(void);

/**
 * @brief Enable tracing
 */
void itd_trace_on(void);

/**
 * @brief Disable tracing
 */
void itd_trace_off(void);

/**
 * @brief Output trace marker
 *
 * Always prints a trace marker, which will appear as a comment in
 * /sys/kernel/debug/tracing/trace. If tracing is not enabled it will be
 * temporarily enabled whilst the marker is written and then disabled after.
 * If tracing is already enable the marker is just written and tracing
 * remains enabled.
 */
void itd_trace_print(const char *const fmt, ...)
    __attribute__((format(printf, 1, 2)));

#endif /* ITD_FTRACE_DEBUGGING_H */
