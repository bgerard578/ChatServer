#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUF_SIZE 500

int main(int argc, char *argv[]) {
	int              sfd, s, bytes;
	char             buf[BUF_SIZE];
	size_t           len;
	ssize_t          nread;
	struct addrinfo  hints;
	struct addrinfo  *result, *rp;

	/* Obtain address(es) matching host/port. */

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_STREAM;/* TCP socket */
	hints.ai_flags = 0;
	hints.ai_protocol = IPPROTO_TCP;/* TCP */

	s = getaddrinfo(argv[1], argv[2], &hints, &result);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

	/* getaddrinfo() returns a list of address structures.
		 Try each address until we successfully connect(2).
		 If socket(2) (or connect(2)) fails, we (close the socket
		 and) try the next address. */

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sfd == -1) { continue; }

		if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1) { break; } /* Success */

		close(sfd);
	}

	freeaddrinfo(result);           /* No longer needed */

	if (rp == NULL) {               /* No address succeeded */
		fprintf(stderr, "Could not connect: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	printf("Client writing ping\n");
	bytes = write(sfd, "ping", 5);
	bytes = read(sfd, buf, 5);
	printf("PID: %d; client received %s\n", getpid(), buf);

	close(sfd);
	return 0;


}
