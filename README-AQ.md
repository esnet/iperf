# Aquantia iperf3 fork

State of development and features description

# Bulk test
Option to run large test suite sequentially.

## Using

`iperf3 -c server --testsuite = ./testsuite1.json`

## testsuite1.json example
```
{
	"coefficients" : {
		"bps" : 1
	},
	"case1" : {
		"options" : "-i 1 -t 10 -i 2",
		"repeat" : 5,
                "averaging" : true,
                "active": false,
                "description": "test description and comments"
	}
}
```
For more information: https://github.com/Aquantia/iperf/issues/2

# Bidirectional mode

Bidirectional traffic run
Server and client are senders and receivers at the same time.
*Accepted in esnet/iperf*

## Using
`iperf3 -c localhost --bidir -P 4`

For more information: https://github.com/esnet/iperf/pull/780

# Multithread

*Accepted in esnet/iperf*

> --multithread
>                   creates a number of threads equal to the number of streams. Each
>                   stream handled independently.
>     
> --thread-affinity
>                   experimental option. Use only with --multithread option.
>                   Sets each thread a affinity depending SO_INCOMING_CPU option.

## Using
`iperf3 -c server -P 4 --multithread`

## Branch
Stable version: https://github.com/Aquantia/iperf/tree/thread
Experimental changes: https://github.com/Aquantia/iperf/tree/x_thread 
For more information: https://github.com/Aquantia/iperf/issues/9

