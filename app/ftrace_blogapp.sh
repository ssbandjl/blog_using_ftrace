#!/bin/bash

##
## Use FTrace when running our test app that reads from our dummy cdev's
## sysfs file. Sanitise the ftrace log and present.
##
## Copyright (C) 2019 IT Dev Ltd.
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License along
## with this program; if not, write to the Free Software Foundation, Inc.,
## 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
##

##
## Initialise veriables.
temporary_file=""
debug_mode=${1:-0}

##
## Trap errors and do cleanup if script errors
cleanup() {
    echo 0 > /sys/kernel/debug/tracing/tracing_on
    if [ -f "$temporary_file" ]; then
        rm -fr "$temporary_file"
    fi
    echo "Error on line $1"
}

trap 'cleanup $LINENO' ERR
set -o pipefail
set -o errtrace
set -o nounset
set -o errexit

##
## We must run as root!
if [ "$(id -u)" -ne 0 ]; then
    echo "### ERROR: You must run this script as root";
    exit 1;
fi

##
## Make sure we are using the correct tracer - function_graph
echo "function_graph" > /sys/kernel/debug/tracing/current_tracer

##
## Clobber trace contents
echo "" > /sys/kernel/debug/tracing/trace

##
## Turn tracing on whilst running our program, then off again afterwards
if [ $debug_mode -ne 0 ]; then 
    ./blog_app_debug
else 
    echo 1 > /sys/kernel/debug/tracing/tracing_on
    ./blog_app
    echo 0 > /sys/kernel/debug/tracing/tracing_on
fi

##
## Figure out which CPU we ran on and then cut out all trace lines not for that CPU.
temporary_file=$(mktemp)

if [ $debug_mode -ne 0 ]; then
    ##
    ## Dump out our trace logs between the "app start" and "app end". You will
    ## see "app start" but "app end" is cut. Also remove all irrelevant CPUs - 
    ## we only need the trace for the one our app is running on.n
    awk '/ITDev: app start/{flag=1}/ITDev: app end/{flag=0}flag' /sys/kernel/debug/tracing/trace > "$temporary_file"
else
    cat /sys/kernel/debug/tracing/trace > "$temporary_file"
fi

cpuno=$(grep "itdev_example_cdev_read_special_data" < "$temporary_file" | sed -re 's/^\s*([0-9]+).*$/\1/g')
grep "^\s*$cpuno" < "$temporary_file"

rm "$temporary_file"

