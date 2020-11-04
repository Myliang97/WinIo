#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include<WinSock2.h>
#include<stdio.h>
#include<windows.h>
#include<openssl/ssl.h>
#include<openssl/err.h>

#pragma comment(lib,"libssl.lib")
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"libcrypto.lib")

#define LISTEN_PORT 8899
#define BUFLEN 1024

DWORD WINAPI ProcessSSLConnect(LPVOID paramer)
{
	SSL *ssl = (SSL*)paramer;
	if (!SSL_accept(ssl))
	{
		ERR_print_errors_fp(stdout);
		printf("\n");
	}
	char buff[BUFLEN];
	while (TRUE)
	{
		memset(buff, 0, BUFLEN);
		size_t size = 0;
		SSL_read_ex(ssl, buff, BUFLEN, &size);
		if (size == 0)
			break;
		printf("[recv]: %s\n", buff);
		char *res = "hello openssl";
		SSL_write(ssl, res, strlen(res));
	}
	SSL_shutdown(ssl);
	SSL_free(ssl);
	return 0;
}

void main()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	SOCKADDR_IN sock_addr;
	sock_addr.sin_addr.S_un.S_addr = htonl(ADDR_ANY);
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(LISTEN_PORT);

	bind(s, (sockaddr *)&sock_addr, sizeof(sock_addr));
	listen(s, 64);

	SSL_library_init();
	OpenSSL_add_all_algorithms();
	SSL_CTX *ctx = SSL_CTX_new(TLSv1_1_server_method());
	SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
	SSL_CTX_load_verify_locations(ctx, "..\\cert\\ca.crt", NULL);
	SSL_CTX_use_certificate_file(ctx, "..\\cert\\server.crt", 1);
	SSL_CTX_use_PrivateKey_file(ctx, "..\\cert\\server2.key", 1);
	if (!SSL_CTX_check_private_key(ctx))
	{
		ERR_print_errors_fp(stdout);
		return;
	}
	while (TRUE)
	{
		SOCKADDR_IN client_addr;
		int len = sizeof(client_addr);
		SOCKET sClient = accept(s, (sockaddr*)&client_addr, &len);
		printf("[hostname]:%s connected\n", inet_ntoa(client_addr.sin_addr));
		SSL *ssl = SSL_new(ctx);
		SSL_set_fd(ssl, sClient);
		CreateThread(NULL, NULL, ProcessSSLConnect,(LPVOID) ssl, 0, NULL);
	}
	closesocket(s);
	SSL_CTX_free(ctx);
	WSACleanup();
}