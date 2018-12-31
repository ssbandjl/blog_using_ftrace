##
## Makefile for A very basic character driver to demonstrate using
## strace to find a bug in the driver.
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
CFLAGS += -Wall -Wextra -Wshadow -Werror -Wconversion -Wsign-conversion -Wcast-align -Wpointer-arith -Wuninitialized -pedantic -pedantic-errors 

cstrings/get_line/get_line.o: cstrings/get_line/get_line.c cstrings/get_line/get_line.h

itd_ftrace_dummy.o: itd_ftrace_dummy.c itd_ftrace_debugging.h
itd_ftrace_debugging.o: itd_ftrace_debugging.c itd_ftrace_debugging.h cstrings/get_line/get_line.h

blog_app: blog_app.o itd_ftrace_dummy.o

blog_app_debug: blog_app.o itd_ftrace_debugging.o cstrings/get_line/get_line.o
	$(LINK.c) $^ -o blog_app_debug

test: CFLAGS += -g
test: test.o cstrings/get_line/get_line.o

.PHONY: clean
clean:
	@$(RM) *.o cstrings/get_line/*.o blog_app blog_app_debug test
