#include <stdio.h>
#include <stdlib.h>

#include "main.h"
#include "http_server.h"

#define MAX_SOCK 32
#define SOCKET_BUFFER_SIZE 4096
#define NO_SOCKET 0xffffffff
#ifndef MAX_PATH
#define MAX_PATH 2048
#endif

struct THttpHandlerListItem
{
	httpHandler handler;
	struct THttpHandlerListItem* next;
};

int defaultHandler(char* page, char* param, int sock);
struct THttpHandlerListItem defaultHandlerListItem = {defaultHandler, NULL};
struct THttpHandlerListItem* HttpHandlerList = &defaultHandlerListItem;

unsigned int listenSock;
struct TSocketInfo
{
	unsigned int socket;
	char buffer[SOCKET_BUFFER_SIZE];
	int size;
} socketInfo[MAX_SOCK];

int defaultHandler(char* page, char* param, int sock)
{
	char buffer[SOCKET_BUFFER_SIZE];
	int size;
	FILE* f;
	char filename[MAX_PATH];
	
	while(page[0] == '/') page++;
	
	snprintf(filename, MAX_PATH, "htdocs/%s", page);
	filename[MAX_PATH - 1] = '\0';
	if (strcmp(filename, "htdocs/") == 0)
        strcpy(filename, "htdocs/index.html");
    f = fopen(filename, "rb");
	if (f)
	{
		while((size = fread(buffer, 1, SOCKET_BUFFER_SIZE, f)) > 0)
		{
			httpSend(sock, buffer, size);
		}
		fclose(f);
	}else{
		httpSend(sock, buffer, sprintf(buffer, "ERROR: file not found."));
	}
	return 1;
}

void registerHttpHandler(httpHandler handler)
{
	struct THttpHandlerListItem* h = (struct THttpHandlerListItem*)malloc(sizeof(struct THttpHandlerListItem));
	h->handler = handler;
	h->next = HttpHandlerList;
	HttpHandlerList = h;
}

void handleSocket(struct TSocketInfo* si)
{
	char outputBuffer[SOCKET_BUFFER_SIZE];
	char *requestEnd, *getEnd, *pathBegin, *pathEnd, *param;
	int size;
	struct THttpHandlerListItem* handler;

	si->buffer[SOCKET_BUFFER_SIZE-1] = 0;	//Make sure the buffer is 0 terminated.
	if (!(requestEnd = strstr(si->buffer, "\r\n\r\n")))
		return;
	if (!(getEnd = strstr(si->buffer, "\r\n")))
		return;
	*getEnd = 0;
	pathBegin = strstr(si->buffer, " ");
	if (!pathBegin && pathBegin < si->buffer + (SOCKET_BUFFER_SIZE / 2))	//We should have a path, and some buffer behind that. Because we want to write in it.
		return;
	pathBegin++;
	if (!(pathEnd = strstr(pathBegin, " ")))
		return;
	*pathEnd = 0;
	//Request OK! Handle it.
	if ((param = strstr(pathBegin, "?")))
		*param++ = 0;

	size = sprintf(outputBuffer, "HTTP/1.1 200 OK\r\n");
	size += sprintf(outputBuffer + size, "Expires: Thu, 01 Dec 1994 16:00:00 GMT\r\n");
	size += sprintf(outputBuffer + size, "Pragma: no-cache\r\n");
	size += sprintf(outputBuffer + size, "Cache-Control: no-store, no-cache, must-revalidate\r\n");
    size += sprintf(outputBuffer + size, "Cache-Control: post-check=0, pre-check=0\r\n");
    size += sprintf(outputBuffer + size, "Transfer-Encoding: chunked\r\n");

	send(si->socket, outputBuffer, size, 0);
	DEBUG(DEBUG_HTTP, "http request %d: %s: %s\n", si->socket, pathBegin, param);
	for(handler = HttpHandlerList; handler; handler = handler->next)
		if (handler->handler(pathBegin, param, si->socket))
			break;
	size = sprintf(outputBuffer, "\r\n0\r\n\r\n");
	send(si->socket, outputBuffer, size, 0);
	si->size = 0;
}

void httpSend(int socket, const void* data, int data_len)
{
    char buffer[32];
    if (data_len < 1)
        return;
    sprintf(buffer, "\r\n%x\r\n", data_len);
    send(socket, buffer, strlen(buffer), 0);
    send(socket, data, data_len, 0);
}

void abort_error(char* error)
{
	perror(error);
	exit(1);
}

void* serverLoop(void* data)
{
	struct timeval tv;
	fd_set   readfds;
	int i, maxSock, newSock;
	struct   sockaddr_in sin;
#ifdef WIN32
	struct WSAData wsaData;
	if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0)
		abort_error("WSAStartup error!");
#endif
	for(i=0;i<MAX_SOCK;i++)
		socketInfo[i].socket = NO_SOCKET;

	if ((listenSock = socket(AF_INET, SOCK_STREAM, 0)) == NO_SOCKET)
		abort_error("socket");
	maxSock = listenSock + 1;

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(80);

	if (bind(listenSock, (struct sockaddr *) &sin, sizeof(sin)) == -1)
		abort_error("bind");
    i = 1;
    if (setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, (char*)&i, sizeof(int)) == -1)
        abort_error("setsockopt");
	if (listen(listenSock, 5) == -1)
		abort_error("listen");

	while (1)
	{
		tv.tv_sec = 10;
		tv.tv_usec = 0;

		FD_ZERO(&readfds);
		FD_SET(listenSock, &readfds);
		for(i=0;i<MAX_SOCK;i++)
		{
			if (socketInfo[i].socket != NO_SOCKET)
				FD_SET(socketInfo[i].socket, &readfds);
		}

		if (select(maxSock, &readfds, NULL, NULL, &tv) > 0)
		{
			if (FD_ISSET(listenSock, &readfds))
			{
				newSock = accept(listenSock, NULL, 0);
				if (maxSock < newSock + 1)
					maxSock = newSock + 1;

				for(i=0;i<MAX_SOCK;i++)
				{
					if (socketInfo[i].socket == NO_SOCKET)
					{
						memset(&socketInfo[i], 0, sizeof(struct TSocketInfo));
						socketInfo[i].socket = newSock;
						break;
					}
				}
				if (i == MAX_SOCK)
					sockClose(newSock);
			}
			for(i=0;i<MAX_SOCK;i++)
			{
				if (socketInfo[i].socket != NO_SOCKET && FD_ISSET(socketInfo[i].socket, &readfds))
				{
					int readCount = recv(socketInfo[i].socket, socketInfo[i].buffer + socketInfo[i].size, SOCKET_BUFFER_SIZE - socketInfo[i].size - 1, 0);
					if (readCount == 0)	//recv will return 0 on a closed socket,
					{		//or, in the case that we request 0 bytes it will also return 0. But then the buffer is full so we should close the socket anyhow.
						sockClose(socketInfo[i].socket);
						socketInfo[i].socket = NO_SOCKET;
					}else{
						socketInfo[i].size += readCount;
						socketInfo[i].buffer[socketInfo[i].size] = '\0';
						handleSocket(&socketInfo[i]);
					}
				}
			}
		}else{	//Timeout, just close all open sockets.
			for(i=0;i<MAX_SOCK;i++)
			{
				if (socketInfo[i].socket != NO_SOCKET)
				{
					sockClose(socketInfo[i].socket);
					socketInfo[i].socket = NO_SOCKET;
				}
			}
		}
	}
}
