thono: stb_image.o stb_image_write.o $(wildcard src/*)
	cc -o thono src/*.c stb_image.o stb_image_write.o -lm -lX11 -lGL

stb_image.o: src/stb_image.h
	cc -DSTB_IMAGE_IMPLEMENTATION -x c -O3 -o stb_image.o -c src/stb_image.h

stb_image_write.o: src/stb_image_write.h
	cc -DSTB_IMAGE_WRITE_IMPLEMENTATION -x c -O3 -o stb_image_write.o -c src/stb_image_write.h
