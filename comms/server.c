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

#define PORT "8888"
#define BUF_SIZE 500
#define MAX_CLIENTS 5

/* Define colors */
#define COLOR_RESET "\x1b[97m"
#define COLOR_GREEN "\x1b[92m"
#define COLOR_RED "\x1b[91m"
#define COLOR_PURPLE "\x1b[95m"
#define COLOR_CYAN "\x1b[96m"

pthread_mutex_t client_lock = PTHREAD_MUTEX_INITIALIZER;

/* sets up client info */
typedef struct{
	int fd;
	char username[32];
} Client;

Client clients[MAX_CLIENTS];
message_saferqueue *msg_queue;

/* sends message to all but the sender*/
void broadcast(const char *msg, int sender_fd){
	pthread_mutex_lock(&client_lock);
	for (int i = 0; i < MAX_CLIENTS; i++){
		if (clients[i].fd > 0 && clients[i].fd != sender_fd){
			write(clients[i].fd, msg, strlen(msg));
		}
	}
	pthread_mutex_unlock(&client_lock);
}

void *dispatcher(void *arg){
	while (1){
		char *msg = message_dequeue(msg_queue);
		broadcast(msg, -1);
		free(msg);
	}
	return NULL;
}

//void *client_handler(void *arg);
//void broadcast(const char *msg, int sender_fd);

/* runs clients on different threads(reading and managing connections)*/
void *client_handler(void *arg){
	int fd = (intptr_t)arg;
	char buf[BUF_SIZE];
	ssize_t n;
	char username[32];
	int client_index = -1;
	n = read(fd, username, sizeof(username) - 1);
	if (n <= 0) { close(fd); return NULL; }
	username[n] = '\0';
	srand(time(NULL) + fd);

	pthread_mutex_lock(&client_lock);
	for (int i = 0; i < MAX_CLIENTS; i++){
		if (clients[i].fd == 0){
			clients[i].fd = fd;
			strncpy(clients[i].username, username, sizeof(clients[i].username));
			client_index = i;
			break;
		}
	}
	pthread_mutex_unlock(&client_lock);

	if (client_index == -1){
		write(fd, COLOR_RED "Serverfull.\n" COLOR_RESET, 17);
		close(fd);
		return NULL;
	}

	Client *client = &clients[client_index];

	/* apperance of message to a client joining the server*/
	char welcome_msg[256];
	snprintf (welcome_msg, sizeof(welcome_msg), COLOR_GREEN "Welcome to the server " COLOR_CYAN "%s" COLOR_GREEN"!" COLOR_PURPLE "\n\nType :help for a list of commands.\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n" COLOR_RESET, client->username);
	write(fd, welcome_msg, strlen(welcome_msg));

	/* apperance of message for client joining the server*/
	char join_msg[BUF_SIZE + 64];
	snprintf(join_msg, sizeof(join_msg), COLOR_CYAN "%s" COLOR_GREEN " has joined the chat!\n" COLOR_RESET "Say hi!\n", client->username);
	message_enqueue(msg_queue, join_msg);

	while ((n = read(fd, buf, BUF_SIZE - 1)) > 0){
		buf[n] = '\0';
		/* apperance of message for clients changing their user name*/
		if (strncmp(buf, ":change_username ", 17) == 0){
			char old_name[32];
			strcpy(old_name, client->username);
			snprintf(client->username, sizeof(client->username), "%s", buf + 17);
			char notice[128];
			snprintf(notice, sizeof(notice), COLOR_CYAN "%s" COLOR_PURPLE " changed their name to "COLOR_CYAN "%s" COLOR_PURPLE ".\n" COLOR_RESET, old_name, client->username);
			message_enqueue(msg_queue, notice);
			continue;
		}
		/* apperance of messages sent by user */
		char full_msg[BUF_SIZE + 128];
		snprintf(full_msg, sizeof(full_msg), COLOR_CYAN "%s:" COLOR_RESET " %s\n", client->username, buf);
		message_enqueue(msg_queue, full_msg);
	}
	close(fd);
	pthread_mutex_lock(&client_lock);
	clients[client_index].fd = 0;
	pthread_mutex_unlock(&client_lock);

	/* apperance of leave message for client */
	char leave_msg[128];
	snprintf(leave_msg, sizeof(leave_msg), COLOR_CYAN "%s" COLOR_RED " has left the server.\n" COLOR_RESET, client->username);
	message_enqueue(msg_queue, leave_msg);
}

int main(int argc, char *argv[]) {
	int sfd, s, new_fd;
	struct addrinfo hints, *res, *rp;
	struct sockaddr_storage client_addr;
	socklen_t client_addrlen = sizeof(client_addr);
	char port[6];

	strncpy(port, argc > 1 ? argv[1] : PORT, 5);
	port[5] = '\0';

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	s = getaddrinfo(NULL, port, &hints, &res);
	if (s != 0){
		perror("getaddrinfo");
		exit(EXIT_FAILURE);
	}

	for (rp = res; rp != NULL; rp = rp->ai_next){
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sfd == -1) continue;
		int optval = 1;
		setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
		if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0) break;
		close(sfd);
	}

	freeaddrinfo(res);

	if (rp == NULL){
		fprintf(stderr, COLOR_RED "Server: failed to bind\n" COLOR_RESET);
		exit (EXIT_FAILURE);
	}

	listen(sfd, MAX_CLIENTS);
	printf("Server listening on port %s...\n", port);

	msg_queue = message_saferqueue_initialize();
	pthread_t dispatcher_thread;
	pthread_create(&dispatcher_thread, NULL, dispatcher, NULL);

	while (1){
		new_fd = accept(sfd, (struct sockaddr *)&client_addr, &client_addrlen);
		pthread_t tid;
		pthread_create(&tid, NULL, client_handler, (void *)(intptr_t)new_fd);
	}

	return 0;
}
