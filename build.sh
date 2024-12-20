#!/bin/sh

set -xe
cc -O3 -o thono src/*.c -lm -lX11 -lGL
