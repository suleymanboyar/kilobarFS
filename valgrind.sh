#!/usr/bin/env sh

set -xe

VFLAGS="--leak-check=full --show-leak-kinds=all --track-origins=yes --malloc-fill=0x40 --free-fill=0x23 -s"
valgrind $VFLAGS ./main
