all	: fcrec convertbin

convertbin	: convertbin.c
	gcc -O3 -Wall -Wextra convertbin.c -lm -o convertbin

fcrec	: fcrec.o fcd.o hid-libusb.o
	gcc -O3 -Wall -Wextra fcrec.o fcd.o hid-libusb.o `pkg-config --libs libusb-1.0` -lm -lpthread -lasound -o fcrec

%.o	: %.c
	gcc -c -O3 -Wall -Wextra $< `pkg-config --cflags libusb-1.0` -o $@

clean	:
	rm -f *.o

distclean dclean	: clean
	rm -f fcrec convertbin
