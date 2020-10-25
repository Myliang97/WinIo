#define  _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include<WinSock2.h>
#include<stdio.h>
#define BUFFLEN 1024
#define LISTEN_PORT 8899
#pragma comment(lib,"ws2_32.lib")
void main()
{
	WSADATA wsaData;
	int ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (ret)
	{
		printf("wsastartup error %d", GetLastError());
		return;
	}
	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET)
	{
		printf("socket error %d", GetLastError());
		return;
	}
	u_long argp = 1;
	ioctlsocket(s, FIONBIO, &argp);  //…Ë÷√socket∑«◊Ë»˚

	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(LISTEN_PORT);
	serverAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	while (1)
	{
		ret = connect(s, (sockaddr *)&serverAddr, sizeof(serverAddr));
		if (SOCKET_ERROR == ret)
		{
			int err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK)
			{
				Sleep(500);
				continue;
			}
		}
		break;
	}
	char buff[BUFFLEN];
	sprintf(buff, "socket hello!");
	while (1)
	{
		int ret = send(s, buff, strlen(buff), 0);
		if (ret == SOCKET_ERROR)
		{
			int err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK)
			{
				Sleep(500);
				continue;
			}
		}
		break;
	}
	memset(buff, 0, sizeof(buff));
	while (1)
	{
		int ret = recv(s, buff, BUFFLEN, 0);
		if (ret == SOCKET_ERROR)
		{
			int err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK)
			{
				Sleep(500);
				continue;
			}
		}
		break;
	}
	printf("[recv from server]:%s", buff);
	closesocket(s);
	WSACleanup();
}