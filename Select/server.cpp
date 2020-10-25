#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include<WinSock2.h>
#include<stdio.h>
#include<windows.h>
#pragma comment(lib,"ws2_32.lib")
#define BUFFLEN 1024
#define LISTEN_PORT 8899
SOCKET sClientArr[FD_SETSIZE];
int clientTotal = 0;

DWORD WINAPI ProcessConn(LPVOID lparameter)
{
	FD_SET fd_read;
	FD_SET fd_write;
	while (1)
	{
		FD_ZERO(&fd_read);
		FD_ZERO(&fd_write);
		for (int i = 0; i < clientTotal;++i)
		{
			FD_SET(sClientArr[i], &fd_read);
			FD_SET(sClientArr[i], &fd_write);
		}
		if (SOCKET_ERROR == select(0, &fd_read, &fd_write, NULL, NULL))
		{
			Sleep(500);
			continue;
		}
		for (int i = 0; i < clientTotal;++i)
		{
			if (FD_ISSET(sClientArr[i],&fd_read))  //read
			{
				WSABUF wsBuf;
				char buf[BUFFLEN] = {0};
				wsBuf.buf = buf;
				wsBuf.len = BUFFLEN;
				DWORD nums = 0;
				DWORD flag = 0;
				if (WSARecv(sClientArr[i],&wsBuf,1,&nums,&flag,NULL,NULL) == SOCKET_ERROR)
				{
					if (WSAEWOULDBLOCK != WSAGetLastError())
					{
						closesocket(sClientArr[i]);
						for (int j = i; j < clientTotal -1;++j)
						{
							sClientArr[j] = sClientArr[j + 1];
						}
						i--;
						clientTotal--;
					}
					continue;
				}
				else
				{
					printf("[recv] from client:%s\n", buf);
				}
			}
			else
			{
				if (FD_ISSET(sClientArr[i], &fd_write))  //连接上的socket则会添加至write set
				{
					char buf[BUFFLEN] = "fd write";
					WSABUF wsBuf;
					wsBuf.buf = buf;
					wsBuf.len = BUFFLEN;
					DWORD num = 0;
					if (WSASend(sClientArr[i],&wsBuf,1,&num,0,NULL,NULL) == SOCKET_ERROR)
					{
						if (WSAEWOULDBLOCK != WSAGetLastError())
						{
							closesocket(sClientArr[i]);
							for (int j = i; j < clientTotal - 1; ++j)
							{
								sClientArr[j] = sClientArr[j + 1];
							}
							i--;
						}
						continue;
					}
				}
			}
		}
	}
	WSACleanup();
	return 0;
}

void main()
{
	WSADATA wsaData;
	int ret =WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (ret)
	{
		printf("wsastartup error %d", GetLastError());
		return;
	}
	SOCKET s;
	s = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);
	if (s == INVALID_SOCKET)
	{
		printf("socket error %d", GetLastError());
		return;
	}
	SOCKADDR_IN sock_addr;
	sock_addr.sin_port = htons(LISTEN_PORT);
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_addr.S_un.S_addr = htonl(ADDR_ANY);
	bind(s, (sockaddr *)&sock_addr, sizeof(sock_addr));
	listen(s, 128);
	DWORD threadId = 0;
	HANDLE hd = CreateThread(NULL, NULL, ProcessConn, NULL, 0, &threadId);
	CloseHandle(hd);
	while (1)
	{
		SOCKADDR_IN sock_in;
		int len = sizeof(sock_in);
		printf("cur size:%d\n", clientTotal);
		if (clientTotal == FD_SETSIZE)
		{
			Sleep(500);
			continue;
		}
		SOCKET sc= accept(s, (sockaddr *)&sock_in,&len);
		u_long NonBlock = 1;
		ioctlsocket(sc, FIONBIO, &NonBlock);
		sClientArr[clientTotal++] = sc;
		char *hostname = inet_ntoa(sock_in.sin_addr);
		printf("[hostname]:%s connected\n", hostname);
	}
	WSACleanup();
}