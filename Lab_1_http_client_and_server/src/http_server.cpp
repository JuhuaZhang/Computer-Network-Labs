#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <assert.h>

// my lib
#include <iostream>

#define BACKLOG 10		 // how many pending connections queue will hold
#define MAXDATASIZE 1024 // max number of bytes we can get at once

using namespace std;

void sigchld_handler(int s)
{
	while (waitpid(-1, NULL, WNOHANG) > 0)
		;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
	{
		return &(((struct sockaddr_in *)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{

	if (argc != 2)
	{
		fprintf(stderr, "usage: port number\n");
		exit(1);
	}

	int sockfd, new_fd; // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes = 1;
	char s[INET6_ADDRSTRLEN];
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	// port
	string input = argv[1];
	int port = stoi(input);
	if ((port > 65535) || (port < 0))
	{
		cout << "incorrect port number" << endl;
		exit(1);
	}

	if ((rv = getaddrinfo(NULL, input.data(), &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next)
	{
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
							 p->ai_protocol)) == -1)
		{
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
					   sizeof(int)) == -1)
		{
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL)
	{
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, BACKLOG) == -1)
	{
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1)
	{
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...\n");

	while (1)
	{ // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1)
		{
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
				  get_in_addr((struct sockaddr *)&their_addr),
				  s, sizeof s);
		printf("server: got connection from %s\n", s);

		if (!fork())
		{				   // this is the child process
			close(sockfd); // child doesn't need the listener
			int numbytes;
			char buf[MAXDATASIZE];
			string header;
			// receive request
			numbytes = recv(new_fd, buf, 1024, 0);
			if (numbytes <= 0)
			{
				// 400 Bad request
				header = "HTTP/1.1 400 Bad Request\r\n\r\n";
				send(new_fd, header.data(), header.length(), 0);
				printf("failed to recieve request\n");
				printf("HTTP/1.1 400 Bad Request\n");
				break;
			}

			// find the file path
			string req, path;
			size_t idx;
			req = string(buf);
			cout << req << endl;

			if (((idx = req.find("/")) == string::npos) || req.substr(0, 3) != "GET")
			{
				header = "HTTP/1.1 400 Bad Request\r\n\r\n";
				send(new_fd, header.data(), header.length(), 0);
				// check send status
				// not write yet
				printf("Invalid file path!\n");
				printf("HTTP/1.1 400 Bad Request\n");
				break;
			}
			path = req.substr(idx, req.length());
			idx = path.find(" ");
			// should check all the request
			// not write yet

			path = path.substr(0, idx);
			path = "./" + path;
			cout << path << endl;

			// open file
			FILE *fp = fopen(path.data(), "rb");
			// if open fail, 404 not found
			if (fp == NULL)
			{
				header = "HTTP/1.1 404 Not Found\r\n\r\n";
				send(new_fd, header.data(), header.length(), 0);
				// check send status
				// not write yet
				printf("Can't find the file!\n");
				cout << path << endl;
				printf("HTTP/1.1 404 Not Found\n");
				break;
			}
			// rewirte
			int header_flag = 1;
			while (!feof(fp))
			{
				numbytes = fread(buf, sizeof(char), MAXDATASIZE, fp);
				if (numbytes > 0)
				{
					if (header_flag)
					{
						header_flag = 0;
						header = "HTTP/1.1 200 OK\r\n\r\n";
						printf("sending...\n");
						send(new_fd, header.data(), header.length(), 0);
					}
					send(new_fd, buf, numbytes, 0);
				}
				else // if (numbytes <= 0)
				{
					break;
				}
			}
			printf("send complete.\n");
			fclose(fp);
			break;
		}
		close(new_fd); // parent doesn't need this
	}

	return 0;
}
