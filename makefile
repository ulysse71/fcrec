all	: fcrec convertasc

convertbin	: convertbin.c
	gcc -O3 -Wall -Wextra convertbin.c -lm -o convertbin

convertasc	: convertasc.c
	gcc -O3 -Wall -Wextra convertasc.c -lm -o convertasc

fcrec	: fcrec.o fcd.o
	gcc -O3 -Wall -Wextra fcrec.o fcd.o `pkg-config --libs hidapi-hidraw` `pkg-config --libs libusb-1.0` -lm -lpthread -lasound -o fcrec

%.o	: %.c
	gcc -c -O3 -Wall -Wextra $< `pkg-config --cflags libusb-1.0` -o $@

clean	:
	rm -f *.o

distclean dclean	: clean
	rm -f fcrec convertbin
