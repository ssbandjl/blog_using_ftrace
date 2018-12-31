#!/bin/bash

##
## A very basic character driver to demonstrate using ftrace to find a bug
## in the driver.
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
## We must run as root!
if [ "$(id -u)" -ne 0 ]; then
    echo "### ERROR: You must run this script as root";
    exit 1;
fi

##
## Remove the module and its /dev file in case it has already been loaded
rmmod most_basic > /dev/null 2>&1
rm -fr /dev/itdev0

##
## Try to load the module
if insmod most_basic.ko; then
    ##
    ## Find the module's major device number and create a file node in /dev
    majnum=$(grep "ITDev Blog" < /proc/devices | sed -re "s/\s*([0-9]+).*/\1/g")
    if [ -z "$majnum" ]; then
        echo "### ERROR: Failed to find device major num"
        rmmod most_basic
        exit 1;
    fi

    if mknod /dev/itdev0 c "$majnum" 0; then
        echo "Module installed"
    else
        echo "### ERROR: Failed to install module"
        rmmod most_basic
        exit 1;
    fi
else
    echo "### ERROR: Failed to install module"
fi
