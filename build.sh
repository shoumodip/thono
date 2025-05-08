#!/bin/sh

set -xe

if [ ! -d .build ]; then
    mkdir .build
	cc -O3 -o .build/stb_image.o -DSTB_IMAGE_IMPLEMENTATION -x c -c src/stb_image.h
	cc -O3 -o .build/stb_image_write.o -DSTB_IMAGE_WRITE_IMPLEMENTATION -x c -c src/stb_image_write.h
	cc -O3 -o .build/stb_image_resize2.o -DSTB_IMAGE_RESIZE2_IMPLEMENTATION -x c -c src/stb_image_resize2.h
    cc -o .build/dump assets/dump.c src/basic.c
fi

.build/dump assets/image.fs image_fs > .build/image_fs.c
.build/dump assets/image.vs image_vs > .build/image_vs.c
.build/dump assets/overlay.fs overlay_fs > .build/overlay_fs.c
.build/dump assets/overlay.vs overlay_vs > .build/overlay_vs.c

cc -O3 -o thono -DRELEASE src/*.c .build/*.c .build/*.o -lm -lGL -lX11
