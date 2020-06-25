CC=gcc

all:
	$(CC) -o dnswatch dnswatch.c -lresolv

install:
	install dnswatch /usr/local/bin/
