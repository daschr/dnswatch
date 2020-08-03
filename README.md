# dnswatch
Wait for changes of DNS A/AAAA records and run a given command each time.

## install
* `make`
* `sudo make install`

## usage
`dnswatch [@<nameserver>|T<stime>|AAAA] [--] [fqdn] [command] [args]...`

## "But for what is it good for?!?"

Well, I use it for wireguard clients connecting to a peer with a dynamic address.

`dnswatch T30 nonexistent.freemyip.com sh -c 'wg-quick down wg0; wg-quick up wg0'`

or

`dnswatch @208.67.222.222 T120 AAAA nonexistent.freemyip.com restart_wireguard.sh`

## more examples

* `dnswatch golem.de sh -c 'echo "changed to $ADDRESSES!"'` <-- will contact the default name server (1.1.1.1) every 60 seconds, executes the command in case of change
* `dnswatch @8.8.8.8 golem.de sh -c 'echo "changed to $ADDRESSES!"'` <-- will contact google's name server every 60 seconds, executes the command in case of change
* `dnswatch @8.8.8.8 T10 golem.de sh -c 'echo "changed to $ADDRESSES!"'` <-- will contact google's name server every 10 seconds, executes the command in case of change
* `dnswatch @8.8.8.8 T10 AAAA golem.de sh -c 'echo "changed to $ADDRESSES!"'` <-- will contact google's name server every 10 seconds, fetches the AAAA record, executes the command in case of change
