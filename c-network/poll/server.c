#define _GNU_SOURCE 1
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <poll.h>
#include <signal.h>

#define USER_LIMIT 5
#define BUFFER_SIZE 64
#define FD_LIMIT 65535

int main(int argc, char *argv[])
{
	if(argc <= 2){
		printf("usage : %s ip_address port\n", argv[0]);
		return 1;
	}

	const char *ip = argv[1];
	int port = atoi(argv[2]);

	struct sockaddr_in address;
	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &address.sin_addr);
	address.sin_port = htons(port);

	int sock = socket(AF_INET, SOCK_STREAM, 0);
	assert(sock >= 0);

	int ret = bind(sock, (struct sockaddr *)&address, sizeof(address));
	assert(ret != -1);

	ret = listen(sock, USER_LIMIT);
	assert(ret != -1);

	struct pollfd fds[USER_LIMIT];
	int user_counter = 0;
	int i;

	fds[0].fd = sock;
	fds[0].events = POLLIN | POLLERR;
	fds[0].revents = 0;

	int g;
	for(g = 1; g < 5; g++){
		fds[g].fd = -1;
		fds[g].events = 0;
	}

	while(1){
		printf("user_counter = %d\n", user_counter);
		int ret = poll(fds, user_counter + 1, -1);
		if(ret == -1){
			printf("poll failured\n");
			continue;
		}else if(ret == 0){
			printf("time out\n");
			continue;
		}

		if(fds[0].revents & POLLIN){
			struct sockaddr_in client_address;
			socklen_t client_addrlength = sizeof(client_address);
			int connfd = accept(sock, (struct sockaddr *)&client_address, &client_addrlength);

			if(connfd < 0){
				printf("errno is %d\n", errno);
				continue;
			}else{
				int m;
				for(m = 1; m < 5; m++){
					if(fds[m].fd < 0){
						user_counter++;
						printf("new client %d comes\n", m);
						fds[m].fd = connfd;
						fds[m].events = POLLIN | POLLERR | POLLRDHUP;
						fds[m].revents = 0;
						break;
					}
				}
				user_counter = user_counter < m ? m : user_counter;
			}
		}

		int k;
		for(k = 1; k < 5; k++){
			if(fds[k].fd < 0){
				continue;
			}

			if(fds[k].revents & POLLRDHUP){
				close(fds[k].fd);
				fds[k].fd = -1;
				user_counter--;
				printf("a client left\n");
			}else if(fds[k].revents & POLLIN){
				char msg[200];
				memset(msg, 0, 200);
				int res = read(fds[k].fd, msg, 200);
				if(res < 0){
					close(fds[k].fd);
					fds[k].fd = -1;
					continue;
				}
				int n = write(STDOUT_FILENO, msg, 200);
				signal(SIGPIPE, SIG_IGN);

				int tmp;
				for(tmp = 1; tmp < 5; tmp++){
					int m = write(fds[tmp].fd, msg, 200);
				}
			}
		}
	}

	close(sock);

	return 0;
}
