all:
	$(CC) -O3 -Wall -pedantic -o dnswatch dnswatch.c -lresolv

deprecated:
	$(CC) -DUSE_DEPRECATED -O3 -Wall -pedantic -o dnswatch dnswatch.c -lresolv

install:
	install dnswatch /usr/local/bin/
