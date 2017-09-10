CC = gcc
CPPFLAGS =
CFLAGS = -std=c11 -D_XOPEN_SOURCE=700 -O2 -Wall -Wextra -Wformat=2
LDFLAGS =
LOADLIBES =
LDLIBS =
OBJFLAGS = -c

.DEFAULT: all
.PHONY: all
all: 
	$(CC) $(CFLAGS) tftpd.c -o tftpd

clean:
	rm -f *.o

distclean: clean
	rm -f tftpd
