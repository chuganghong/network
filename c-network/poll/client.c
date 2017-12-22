#define _GNU_SOURCE 1
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <poll.h>
#include <fcntl.h>

#define BUFFER_SIZE 64
#define BUF_SIZE 1024

int main(int argc, char *argv[])
{
	printf("%d\n", argc);
	if(argc <= 2){
		printf("usage: %s ip port\n", argv[0]);
		return 0;
	}

	const char *ip = argv[1];
	int port = atoi(argv[2]);
	char message[BUF_SIZE];

	struct sockaddr_in address;
	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &address.sin_addr);
	address.sin_port = htons(port);
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	assert(sock >= 0);

	int conn = connect(sock,(struct sockaddr*)&address, sizeof(address));
	if(conn == -1){
		printf("connection failed\n");
		close(sock);

		return 1;
	}

	struct pollfd fds[2];
	fds[0].fd = sock;
	fds[0].events = POLLIN | POLLRDHUP;
	fds[0].revents = 0;

	fds[1].fd = STDIN_FILENO;
	fds[1].events = POLLIN;
	while(1){
		int ret = poll(fds, 2, -1);
		if(fds[0].revents & POLLRDHUP){
			printf("server close the connection\n");
			break;
		}

		if(fds[0].revents & POLLIN){
			char msg[200];
			memset(msg, 0, 200);
			int res = read(sock, msg, 200);
			if(res == 0){
				fprintf(stderr, "client: server is closed.\n");
				close(sock);
			}
			write(STDOUT_FILENO, msg, 200);
		}else{
			printf("sock io is not ready\n");
		}

		if(fds[1].revents & POLLIN){
			char sendline[200];
			memset(sendline, 0, 200);
			int res = read(STDIN_FILENO, sendline , 200);
			if(res == 0){
				shutdown(sock, SHUT_WR);
				continue;
			}
			write(sock, sendline ,200);
		}else{
			printf("stdin is not readable\n");
		}
	}

	return 0;
}

