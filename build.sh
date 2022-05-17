#!/bin/sh -xe
cc -Wall -Wextra -std=c11 -pedantic -o thono thono.c -lXrender -lX11 -ljpeg
