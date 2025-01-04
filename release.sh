#!/bin/sh

set -xe

if [ ! -d .build ]; then
    mkdir .build
	cc -O3 -o .build/stb_image.o -DSTB_IMAGE_IMPLEMENTATION -x c -c src/stb_image.h
	cc -O3 -o .build/stb_image_write.o -DSTB_IMAGE_WRITE_IMPLEMENTATION -x c -c src/stb_image_write.h
	cc -O3 -o .build/stb_image_resize2.o -DSTB_IMAGE_RESIZE2_IMPLEMENTATION -x c -c src/stb_image_resize2.h
fi

sed 's/"/\\"/g;s/^/"/;s/$/\\n"/' shaders/image.fs > .build/image_fs
sed 's/"/\\"/g;s/^/"/;s/$/\\n"/' shaders/image.vs > .build/image_vs
sed 's/"/\\"/g;s/^/"/;s/$/\\n"/' shaders/overlay.fs > .build/overlay_fs
sed 's/"/\\"/g;s/^/"/;s/$/\\n"/' shaders/overlay.vs > .build/overlay_vs

cc -O3 -o thono -DRELEASE src/*.c .build/*.o -lm -lGL -lX11
