#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include<windows.h>
#include<stdio.h>
#include<openssl/ssl.h>
#include <openssl/err.h>

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"libssl.lib")
#pragma comment(lib,"libcrypto.lib")
#define LISTEN_PORT 8899
#define BUFLEN 1024

void main()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	SOCKADDR_IN sock_addr;
	sock_addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(LISTEN_PORT);

	SSL_library_init();
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();
	SSL_CTX *ctx = SSL_CTX_new(TLSv1_1_client_method());
	SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
	SSL *ssl = SSL_new(ctx);
	connect(s, (sockaddr*)&sock_addr, sizeof(sock_addr));
	SSL_set_fd(ssl, s);
	if (!SSL_connect(ssl))
	{
		ERR_print_errors_fp(stdout);
		printf("\n");
	}
	char buf[] = "come from ssl client";
	size_t size = 0;
	if (!SSL_write_ex(ssl, buf, strlen(buf), &size))
	{
		ERR_print_errors_fp(stdout);
	}
	printf("write:%d\n", size);
	char recvBuf[BUFLEN];
	memset(recvBuf, 0, sizeof(recvBuf));
	SSL_read(ssl, recvBuf, BUFLEN);
	printf("[recv]: %s\n", recvBuf);
	SSL_shutdown(ssl);
	SSL_free(ssl);
	SSL_CTX_free(ctx);
	closesocket(s);
	WSACleanup();
}