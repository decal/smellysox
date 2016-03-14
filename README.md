# smellysox by Derek Callaway (datapipe.c port)

A rendition of the old school classic datapipe.c ported to contemporary Linux..
Tested on Linux kernel v3.3.x

This is essentially a "port-forward" of a TCP port forwarding program..
Provide a local port, remote host and remote port on the command line..

Connecting to the local port while the program is running will create I/O streams
which make the remote destination accessible..

The socket code for the original was getting rather stale so I updated it with a 
much needed revision..

