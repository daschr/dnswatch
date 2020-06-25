CC=gcc

all:
	$(CC) -O3 -Wall -pedantic -o dnswatch dnswatch.c -lresolv

install:
	install dnswatch /usr/local/bin/
