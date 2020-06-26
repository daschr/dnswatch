# dnswatch
Wait for changes of DNS A records and run a given command each time.

## install
* `make`
* `sudo make install`

## usage
`dnswatch [@<nameserver>|T<stime>] [--] [fqdn] [command] [args]...`

## examples

* `dnswatch golem.de sh -c 'echo "changed to $ADDRESS!"'` <-- will contact the default name server (1.1.1.1) every 60 seconds, executes the command in case of change
* `dnswatch @8.8.8.8 golem.de sh -c 'echo "changed to $ADDRESS!"'` <-- will contact google's name server every 60 seconds, executes the command in case of change
* `dnswatch @8.8.8.8 T10 golem.de sh -c 'echo "changed to $ADDRESS!"'` <-- will contact google's name server every 10 seconds, executes the command in case of change

## TODO

* option for ipv6
* maybe compare all A/AAAA records, for now, the tool only compares the first one
