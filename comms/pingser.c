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
	int			 reuse_port = 1;
	int			 connection;
	int                      sfd, s;
	char                     buf[BUF_SIZE];
	char			 portnum[6];
	ssize_t                  nread;
	socklen_t                peer_addrlen;
	struct addrinfo          hints;
	struct addrinfo          *result, *rp;
	struct sockaddr_storage  peer_addr;

	if (argc > 1) {
		strncpy(portnum, argv[1], 5);
		portnum[5] = '\0';
	} else {
		strcpy(portnum, "8888");
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_STREAM;/* TCP protocol socket */
	hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
	hints.ai_protocol = IPPROTO_TCP;/* TCP */
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	s = getaddrinfo(NULL, portnum, &hints, &result);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

	/* getaddrinfo() returns a list of address structures.
	   Try each address until we successfully bind(2).
	   If socket(2) (or bind(2)) fails, we (close the socket
	   and) try the next address. */

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sfd == -1) { continue; }

		/* Allow reuse of the port--spread the accept load out across multiple threads */
		if (s = setsockopt(sfd, SOL_SOCKET, SO_REUSEPORT, &reuse_port, sizeof(reuse_port)) < 0) {
			fprintf(stderr, "Error on setsockopt: %s\n", strerror(errno));
			continue;
		}

		if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0) { break; } /* Success */

		close(sfd);
	}

	freeaddrinfo(result);           /* No longer needed */

	if (rp == NULL) {               /* No address succeeded */
		fprintf(stderr, "Could not bind\n");
		exit(EXIT_FAILURE);
	}

	/* Listen to the socket--this is what allows delegating sessions to other file descriptors (and threads) */
	if (listen(sfd, 1) < 0) {
		fprintf(stderr, "Error in listen: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	printf("Calling accept\n");
	connection = accept(sfd, NULL, NULL);
	if (connection < 0) {
		fprintf(stderr, "Error in accept: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* Do the ping-pong thing */
	read(connection, buf, 5);
	printf("PID: %d; server received %s\n", getpid(), buf);
	strcpy(buf, "pong");
	printf("Server writes %s\n", buf);
	write(connection, buf, 5);

	close(connection);			/* Tear down the session with client */

	return 0;
}
