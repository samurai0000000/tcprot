This software is designed as a simple measure to obfusticate data and bypass
DPI firewalls. This is effective against ones that monitor TCP traffic
and drop connections that contain ssh protocol headers. A simple (and low in
computational overhead) byte level transformation is sufficient in most cases.

Deployment:
	- Place TUN1 on ssh client side
	- Place TUN2 on ssh server side
	- TUN1 connects to TUN2 and tunneled traffic are transformed

  +-------+       +----+                   +----+     +------ +
  |  SSH  + --->  |TUN1| -->  DPI FW   --> |TUN2| --> |  SSH  |
  | Client|       |    |                   |    |     | Server|
  +-------+       +----+                   +----+     +-------+

  ports:           42022                    42021           22

TODO:
	- Custom nyte stream transformation as need arise
	- connect() is a blocking call and can be improved with fcntl and made
	  non-blocking
	- statistics support
	- logging supoprt
