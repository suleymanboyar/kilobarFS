#!/usr/bin/env sh

set -xe

CFLAGS="-Wall -Wextra -ggdb -pedantic"
CC="gcc"
CLIBS="-lm"
$CC $CFLAGS $CLIBS -o main main.c
