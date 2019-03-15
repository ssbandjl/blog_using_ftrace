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

/*
 * DESCRIPTION:
 *   This is a dummy application that will exhibit a specific behaviour. The
 *   imagined driver author has made a small error: When the driver's only
 *   binary file is read more than once in succession, after the 1st read,
 *   no data is ever returned. This was not the driver's intent. We are
 *   imagining that the device always has some informaton to give us, so the
 *   file should essentially be "infinite" in length.
 *
 *   We will be able to use ftrace to debug this problem as described in the
 *   blog to which this code is associated.
 *
 *   To help use ftrace from within our application a very small and simple
 *   library is used to make our lives a little easier. This library hides the
 *   details of what we're doing with frace, but provides a nice little API
 *   should you ever wish to use ftrace in this manner.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include "itd_ftrace_debugging.h"

#define SPECIAL_DATA_BLOCK_SIZE 22U
#define TAG "ITDev: "

/*
 * Read one block of "binary" data from the drivers "special" file.
 *
 * We're really only imagining that this is binary data as our toy driver
 * example will return us a string of length SPECIAL_DATA_BLOCK_SIZE
 * characters. We print out that string.
 */
static ssize_t print_special_data_block(const int special_file_fh)
{
    static char buffer[SPECIAL_DATA_BLOCK_SIZE + 1];

    itd_trace_on();
    itd_trace_print(TAG "Reading special file from app\n");

    /* Ignores CWE-120,CWE-20 - I can't see a vunerability? The buffer size being read is
     * greater than SPECIAL_DATA_BLOCK_SIZE and I check the return value of the call */
    /* Flawfinder: ignore */
    const ssize_t bytes_read = read(special_file_fh, buffer, SPECIAL_DATA_BLOCK_SIZE); 

    itd_trace_off();

    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        printf("Read \"%s\"\n", buffer); /* Would be binary but example driver */
                                         /* returns ASCII for ease             */
    }
    else if (bytes_read == 0) {
        printf("EOF.\n");
    }
    else {
        fprintf(stderr, "Failed to read device!\n");
    }

    return bytes_read;
}

int main(void)
{
    int ret_val = EXIT_FAILURE;
    unsigned int loop_counter = 0;
    const int special_file_fh =
        open("/sys/devices/itdev/special_data", O_RDONLY | O_NONBLOCK);

    if (special_file_fh < 0) {
        perror("Failed to open driver special_file");
        goto exit_main;
    }

    if (itd_init_debug_tracing() < 0) {
        perror("Failed to initialise debug tracing");
        goto exit_main;
    }

    itd_trace_print(TAG "app start\n");

    /*
     * Read from the file twice. The driver writers intent was that data
     * would be constantly generated so we expect to always read out data.
     * However, with the driver as provided, only the first read will
     * succeed and it is up to us to figure out why!
     */
    for (; loop_counter < 2; ++loop_counter) {
        const ssize_t result = print_special_data_block(special_file_fh);

        if (result <= 0)
            break;
    }

    itd_trace_print(TAG "app end\n");
    itd_uninit_debug_tracing();

    close(special_file_fh);
    ret_val = EXIT_SUCCESS;

exit_main:
    return ret_val;
}
