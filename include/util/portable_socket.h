#ifdef WIN32

#include <Windows.h> 
#pragma comment(lib, "ws2_32.lib")

void socket_init() {
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
}

void socket_destroy() {
	WSACleanup();
}

#else

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> /* close */
#include <netdb.h> /* gethostbyname */
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

void socket_init() {}
void socket_destroy() {}

#endif