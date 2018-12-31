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
#include "itd_ftrace_debugging.c" /*< Note C include! */
#include "cstrings/get_line/get_line.h"

int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	allocate_and_set_tracefs_file_paths("/the/test/path", false);
	printf("Test: %s\n      %s\n", tracing_on_file_path, trace_marker_file_path);
	free(tracing_on_file_path);
	free(trace_marker_file_path);
	tracing_on_file_path = NULL;
	trace_marker_file_path = NULL;

	allocate_and_set_tracefs_file_paths("/the/test/path", true);
	printf("Test: %s\n      %s\n", tracing_on_file_path, trace_marker_file_path);
	free(tracing_on_file_path);
	free(trace_marker_file_path);
	tracing_on_file_path = NULL;
	trace_marker_file_path = NULL;

	find_tracefs();
	printf("Test: %s\n      %s\n", tracing_on_file_path, trace_marker_file_path);
	free(tracing_on_file_path);
	free(trace_marker_file_path);
	tracing_on_file_path = NULL;
	trace_marker_file_path = NULL;

	return 0;
}
