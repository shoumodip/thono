LIBS=x11 xrender libjpeg
CFLAGS=-Wall -Wextra -std=c11 -pedantic `pkg-config --cflags $(LIBS)`

thono: thono.c
	$(CC) $(CFLAGS) -o thono thono.c `pkg-config --libs $(LIBS)`
