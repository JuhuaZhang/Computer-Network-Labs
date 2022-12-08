#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <assert.h>

#include <arpa/inet.h>

// my lib
#include <iostream>

#define MAXDATASIZE 1024 // max number of bytes we can get at once

using namespace std;

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
	{
		return &(((struct sockaddr_in *)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

// resolve input to domain + path + port
void *resolve_input(string *input, string *domain, string *path, string *port)
{
	string url;
	// get http addr
	size_t idx = 0;
	idx = (*input).find("http://");
	if (idx == string::npos)
	{
		printf("Can't find 'http://' prefix \n");
		exit(1);
	}
	else
	{
		url = (*input).substr(idx + 7, (*input).length());
	}
	// Get the domain and the file path
	idx = url.find("/");
	if (idx == string::npos)
	{
		printf("Can't find file \n");
		exit(1);
	}
	else
	{
		*domain = url.substr(0, idx);
		*path = url.substr(idx); // not sure if is needed +1
	}
	// find port num
	idx = (*domain).find(":");
	if (idx != string::npos)
	{
		*port = (*domain).substr(idx + 1); // there's ':' in front of port num
		*domain = (*domain).substr(0, idx);
	}
	return 0;
}

int main(int argc, char *argv[])
{
	int sockfd, numbytes;
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p; // hints hold the socket addr info
	int rv;
	char s[INET6_ADDRSTRLEN];

	if (argc != 2)
	{
		fprintf(stderr, "usage: http://hostname[:port]/file_path\n");
		exit(1);
	}

	// Empty the struct
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	// split the http preface, the ip addr, the port and file name
	string input = argv[1], domain, path, port = "80";

	resolve_input(&input, &domain, &path, &port);

	if ((rv = getaddrinfo(domain.data(), port.data(), &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}
	// loop through all the results and connect to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next)
	{
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
							 p->ai_protocol)) == -1)
		{
			perror("client: socket"); // print error msg
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(sockfd);
			perror("client: connect");
			continue;
		}
		break;
	}

	if (p == NULL)
	{
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	// convert ip_addr to string
	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			  s, sizeof s);
	printf("client: connecting to %s\n", s);
	freeaddrinfo(servinfo); // all done with this structure

	// send request
	string http_req; // GET request
	// http_req = "GET " + path + " HTTP/1.1\r\n\r\n";
	// alternative
	http_req = "GET " + path + " HTTP/1.1\r\n" +
			   "User-agent: Wget/1.12 (linux-gnu)\r\n" +
			   "Host: " + domain + ":" + port + "\r\n" +
			   "Connection: Keep-Alive\r\n\r\n";
	size_t sent_size;
	sent_size = send(sockfd, http_req.data(), http_req.size(), 0);

	// if send failed
	if (sent_size < 0)
	{
		printf("failed to send request \n");
		exit(1);
	}
	// not sure whether it works
	// else if (sent_size == EMSGSIZE)
	// {
	// 	printf("message is too long \n");
	// 	exit(1);
	// }

	// recieve files
	int header_flag = 1;
	printf("save file in ");
	string fname("./output");
	FILE *pf = fopen(fname.c_str(), "w");
	cout << fname << endl;
	while (1)
	{
		numbytes = recv(sockfd, buf, 1024, 0); // size of the file
		if (numbytes > 0)
		{
			// remove header, comment if not required
			if (header_flag)
			{
				header_flag = 0;
				char *data = strstr(buf, "\r\n\r\n") + sizeof("\r\n\r\n") - 1;
				int datasize = numbytes - int(data - &buf[0]);
				string header(buf);
				header = header.substr(0, int(data - &buf[0]));
				cout << header;
				printf("receiving...\n");
				fwrite(data, datasize, 1, pf);
			}
			else
			{
				// printf("receiving \n");
				fwrite(buf, numbytes, 1, pf);
			}
		}
		if (numbytes == 0) // done
		{
			printf("receive complete.\n");
			break;
		}
		if (numbytes < 0) // error
		{
			printf("receive error.\n");
			exit(1);
		}
	}
	fclose(pf);

	// close sock
	close(sockfd);

	return 0;
}
