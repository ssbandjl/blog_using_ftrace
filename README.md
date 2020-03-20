# ITDev Blog: Using FTrace Example Source Code

Source code for a blog about using ftrace to debug an imaginary driver bug.

NOTE: This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY as per the license,
so please have a read through the source code to make sure you know what you are running.

The associated blog gives an example as to how to use ftrace from your driver, from the command line and user space
application in order to debug driver issues.

In this example we are imagining that the cdev driver creates a sysfs binary file for the device which is used to read
back some data. Maybe it is special status information, for example. It doesn't matter. What matters is that there is a
bug in the driver code. The author intends for the binary file to *always* provide some data back to the user. It is
as-if it were an infinite file. However, upon reading this file we will find that only one block of data is returned
and from then on, nothing. See the blog for more information - visit www.itdev.co.uk.

## Setting up the repo
This repo uses submodules. When you clone it remember to use
```bash
git submodule init
git submodule update
```

## Compiling the Driver

The driver is just a basic character device driver. This means that you won't need any special hardware with which
to compile and run it, if you have access to a Linux machine with sudo privileges. This has only been tested
on Ubuntu.

Install the kernel headers on an Ubuntu distro type the following.

```bash
apt-get install linux-headers-$(uname -r) libelf-dev
```

You can now go to the `driver` directory and type `make` to build the driver. The module should be built as 
`most_basic.ko`. To load the driver, use the `start.sh` script. This will remove any already loaded versions of this
module and its /dev/itdev0 pseudo file and reload/recreate.

## Compiling the App

Just go to the `app` directory and type either:

```bash
make blog_app  # to make a "release" app or 
make blog_app_debug # to generate a debug version.
```
