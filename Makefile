LIBS=x11 xrender libjpeg
CFLAGS=-Wall -Wextra -std=c11 -pedantic `pkg-config --cflags $(LIBS)`

thono: $(wildcard src/*)
	$(CC) $(CFLAGS) -o thono src/*.c -lXrender -lX11 -ljpeg
