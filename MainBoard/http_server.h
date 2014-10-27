#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#ifdef WIN32
#include <windows.h>
#define sockClose closesocket
#else
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#define sockClose close
#include <string.h>
#endif

typedef int (*httpHandler)(char* page, char* param, int socket);

//Run the http server, void*(void*) so we can start it from pthread_create
void* serverLoop(void* data);
void registerHttpHandler(httpHandler handler);
void httpSend(int socket, const void* data, int data_len);

#endif//HTTP_SERVER_H
