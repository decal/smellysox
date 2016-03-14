/* 

The following is an interesting snippet of code I wrote recently. It 
makes a data pipe between a listen port on the machine it's being run on 
and a port on a remote machine. For example, running
  datapipe 2222 23 your.machine.com

would create a port 2222 on the local machine that, if telnetted to, would
be the same as telnetting to port 23 on your.machine.com. This can be used
for a variety of purposes: redirect IRC connections so that identd shows
the username of the datapipe process; redirect sendmail direct connections
for the same reason; even use on a firewall machine to give access to an
internal service (ftpd, for instance). Cascaded datapipes make for
interesting traceback dilemmas. Questions and comments accepted.

Compile with:
    cc -o datapipe -O datapipe.c
On boxes without strerror() (like SunOS 4.x), compile with:
    cc -o datapipe -O -DSTRERROR datapipe.c

Run as:
    datapipe localport remoteport remotehost

It will fork itself into the background.

 * Datapipe - Create a listen socket to pipe connections to another
 * machine/port. 'localport' accepts connections on the machine running    
 * datapipe, which will connect to 'remoteport' on 'remotehost'. Fairly
 * standard 500 xxxx extended errors are used if something drastic
 * happens.
 *
 * (c) 1995 Todd Vierling
 *
 * Define STRERROR while compiling on a SunOS 4.x box
 */

/* Modified by Derek Callaway to run on modern Linux, tested on kernel v3.3.x */

#define _GNU_SOURCE 1
#define _XOPEN_SOURCE 1
#include<stdio.h>
#include<sys/timeb.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/wait.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<stdbool.h>
#include<sysexits.h>
#include<errno.h>
#include<unistd.h>
#include<netdb.h>
#include<string.h>

static void vexit(const char *afn) {
  perror(afn);

	exit(EXIT_FAILURE);
}

static void usage(const char *av0) {
  fprintf(stderr, "usage: %s localport remoteport remotehost\n", av0);

	exit(EX_USAGE);
}

#define READ_FACTOR 0x4
#define WRITE_FACTOR 0x4
#define SWAB_FACTOR 0x8

int main(int argc, char *argv[]) {
  struct sockaddr_in laddr, caddr, oaddr;
	socklen_t caddrlen = sizeof caddr;
  int lsock, csock, osock, nbyte;
  FILE *cfile;
  char abuf[4096] = { 0x0, }, *rfry = 0x0, *wfry = 0x0;
  fd_set fdsr, fdse;
  struct hostent *h;
  unsigned long a;
  unsigned short oport;

  if (argc != 4)
	  usage(*argv);

  a = inet_addr(argv[3]);

  if(!(h = gethostbyname(argv[3])) && !(h = gethostbyaddr(&a, 4, AF_INET)))
	  vexit("gethostbyname");

  oport = atol(argv[2]);
  laddr.sin_port = htons((unsigned short)(atol(argv[1])));
  lsock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	if(lsock < 0)
	  vexit("socket");

  laddr.sin_family = PF_INET;
  laddr.sin_addr.s_addr = INADDR_ANY;

  if(bind(lsock, (const struct sockaddr*)&laddr, sizeof laddr))
	  vexit("bind");

  if(listen(lsock, 1)) 
	  vexit("listen");

  if((nbyte = fork()) < 0)
	  vexit("fork");

  if(nbyte > 0)
    return 0;

  setsid();

  while((csock = accept(lsock, (struct sockaddr*)&caddr, &caddrlen)) > -1) {
    cfile = fdopen(csock, "r+");

    if ((nbyte = fork()) < 0) {
      fprintf(cfile, "fork: %s\n", strerror(errno));
      shutdown(csock, SHUT_RDWR);
      fclose(cfile);

      continue;
    }

    if(!nbyte)
      goto gotsock;

    fclose(cfile);

    while(waitpid(-1, NULL, WNOHANG) > 0);
  }

  return 20;

gotsock:
  osock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	if(osock < 0) 
	  vexit("socket");

  oaddr.sin_family = h->h_addrtype;
  oaddr.sin_port = htons(oport);

  memcpy(&oaddr.sin_addr, h->h_addr, h->h_length);

  if(connect(osock, (const struct sockaddr*)&oaddr, sizeof oaddr)) 
	  vexit("connect");

  do {
	  struct timeb mftime = { 0x0, };

    FD_ZERO(&fdsr);
    FD_ZERO(&fdse);
    FD_SET(csock,&fdsr);
    FD_SET(csock,&fdse);
    FD_SET(osock,&fdsr);
    FD_SET(osock,&fdse);

		rfry = wfry = NULL;

    if(select(20, &fdsr, NULL, &fdse, NULL) < 0)
		  vexit("select");

    if(FD_ISSET(csock, &fdsr) || FD_ISSET(csock, &fdse)) {
      nbyte = read(csock, abuf, sizeof abuf);

			if(nbyte < 1)
				vexit("read");

      if(ftime(&mftime) < 0)
			  vexit("ftime");

			srand(mftime.millitm);

			if(!(rand() % READ_FACTOR)) {
				rfry = strfry(abuf);

				if(rfry) {
					nbyte = strlen(rfry);
				
				  if(!(rand() % SWAB_FACTOR)) {
				    swab(rfry, abuf, nbyte);

						rfry = NULL;
					}
				}
			} 

      if(write(osock, rfry ? rfry : abuf, nbyte) < 1)
				vexit("write");
    } else if (FD_ISSET(osock, &fdsr) || FD_ISSET(osock, &fdse)) {
      nbyte = read(osock, abuf, sizeof abuf);

			if(nbyte < 1)
			  vexit("read");

			if(ftime(&mftime) < 0)
			  vexit("ftime");

			srand(mftime.millitm);

      if(!(rand() % WRITE_FACTOR)) {
				wfry = strfry(abuf);

				if(wfry) {
					nbyte = strlen(wfry);

					if(!(rand() % SWAB_FACTOR)) {
					  swab(wfry, abuf, nbyte);

						wfry = NULL;
					}
				}
			}

      if(write(csock, wfry ? wfry : abuf, nbyte) < 1)
			  vexit("write");
    }
  } while(true);

  shutdown(osock, SHUT_RDWR);
  close(osock);

  fflush(cfile);

  shutdown(csock, SHUT_RDWR);

  fclose(cfile);
  
	exit(EXIT_SUCCESS);
}

