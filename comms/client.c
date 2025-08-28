#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include "saferq.h"

#define BUF_SIZE 500

/* Define colors */
#define COLOR_RESET "\x1b[97m"
#define COLOR_GREEN "\x1b[92m"
#define COLOR_RED "\x1b[91m"
#define COLOR_PURPLE "\x1b[95m"
#define COLOR_CYAN "\x1b[96m"

char username[32];
char username[32] = "Anonymous";

void *send_loop(void *arg);
void *recv_loop(void *arg);

int main(int argc, char *argv[]) {
	if (argc < 3){
		fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
		exit(1);
	}

	int sfd, s;
	struct addrinfo hints, *res, *rp;

	/* ask for username */
	printf("Enter a username: ");
	fgets(username, sizeof(username), stdin);
	username[strcspn(username, "\n")] = '\0';

	/* start connection to server */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	s = getaddrinfo(argv[1], argv[2], &hints, &res);
	if (s != 0){
		perror("getaddrinfo");
		exit(EXIT_FAILURE);
	}

	for (rp = res; rp != NULL; rp = rp->ai_next){
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sfd == -1) continue;
		if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1) break;
		close(sfd);
	}

	freeaddrinfo(res);

	if (rp == NULL){
		fprintf(stderr, COLOR_RED "Connection failed\n" COLOR_RESET);
		exit(EXIT_FAILURE);
	}

	/* send username to server */
	write(sfd, username, strlen(username));

	printf(COLOR_GREEN "Connection successful.\n" COLOR_RESET);

	pthread_t send_t, recv_t;
	pthread_create(&send_t, NULL, send_loop, &sfd);
	pthread_create(&recv_t, NULL, recv_loop, &sfd);
	pthread_join(send_t, NULL);
	pthread_join(recv_t, NULL);

	close(sfd);
	return 0;
}

/* Starts its own thread to deal with user input. Deals with commands (starting with ':') and handles commands*/
void *send_loop(void *arg){
	int fd = *(int *)arg;
	char buf[BUF_SIZE];

	while (fgets(buf, BUF_SIZE, stdin) != NULL){
		buf[strcspn(buf, "\n")] = '\0';

		if (buf[0] == ':'){
			if (strncmp(buf, ":quit", 5) == 0){
				printf(COLOR_RED "Server left\n" COLOR_RESET);
				exit(0);
			}
			else if (strncmp(buf, ":help", 5) == 0){
				/* clear what the user just typed */
				printf("\x1b[1F");
				printf("\x1b[2K");

				printf(COLOR_PURPLE "Commands:\n");
				printf(":change_username [name]		Allows you to change your username\n");
				printf(":clear				Clears the previous messages\n");
				printf(":help				Shows a list of commands\n");
				printf(":quit				Leaves the server\n" COLOR_RESET);
				continue;
			}
			else if (strncmp(buf, ":change_username ", 17) == 0){
				snprintf(username, sizeof(username), "%s", buf + 17);

				/* clear what the user just typed */
				printf("\x1b[1F");
				printf("\x1b[2K");

				printf(COLOR_GREEN "Your username has been updated.\n" COLOR_RESET);
				write(fd, buf, strlen(buf));
				continue;
			}
			else if (strncmp(buf, ":clear", 6) == 0){
				/* clear what the user just typed */
				printf("\x1b[1F");
				printf("\x1b[2K");

				printf(COLOR_RESET "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
				continue;
			}
			else{
				/* clear what the user just typed */
				printf("\x1b[1F");
				printf("\x1b[2K");
				
				printf(COLOR_RED "Unknown command. Try " COLOR_PURPLE ":help" COLOR_RED " to see a list of valid commands\n" COLOR_RESET);
				continue;
			}
		}
		write(fd, buf, strlen(buf));
		
		/* clear what the user just typed */
		printf("\x1b[1F");
		printf("\x1b[2K");
	}
	return NULL;
}

/* Starts its own thread to read and display things received from the server */
void *recv_loop(void *arg){
	int fd = *(int *)arg;
	char buf[BUF_SIZE];
	ssize_t n;

	while ((n = read(fd, buf, BUF_SIZE - 1)) > 0){
		buf[n] = '\0';
		printf("%s", buf);
		fflush(stdout);
	}
	return NULL;
}
