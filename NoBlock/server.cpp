#define  _WINSOCK_DEPRECATED_NO_WARNINGS
#include<WinSock2.h>
#include<stdio.h>
#include<windows.h>
#pragma comment(lib,"ws2_32.lib")
#define BUFFLEN 1024
#define LISTEN_PORT 8899
typedef struct  _LISTENINFO
{
	SOCKET s;
	SOCKADDR_IN addr_in;
}LISTENINFO,*PLISTENINFO;

DWORD WINAPI ProcessConn(LPVOID lparameter)
{
	if (!lparameter)
	{
		return 0;
	}
	PLISTENINFO pListen_info = (PLISTENINFO)lparameter;
	char *hostname = inet_ntoa(pListen_info->addr_in.sin_addr);
	printf("[hostname]:%s connected\n", hostname);
	char buff[BUFFLEN] = { 0 };
	while (1)
	{
		int ret =recv(pListen_info->s, buff, BUFFLEN, 0);
		if (ret == SOCKET_ERROR)
		{
			int err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK)
			{
				Sleep(500);
				continue;
			}
			else
			{
				printf("recv failed\n");
				break;
			}
		}
		printf("[recv data]:%s\n", buff);
		ret =send(pListen_info->s, buff, sizeof(buff), 0);
		if (ret == SOCKET_ERROR)
		{
			int err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK)
			{
				Sleep(500);
				continue;
			}
			else
			{
				printf("send failed \n");
				break;
			}
		}
		printf("send data successed\n");
		break;
	}

	delete pListen_info;
	closesocket(pListen_info->s);
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
	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET)
	{
		printf("socket error %d", GetLastError());
		return;
	}
	u_long argp = 1;
	ioctlsocket(s, FIONBIO, &argp);  //ÉèÖÃsocket·Ç×èÈû

	SOCKADDR_IN sock_addr;
	sock_addr.sin_port = htons(LISTEN_PORT);
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_addr.S_un.S_addr = htonl(ADDR_ANY);
	ret =bind(s, (sockaddr *)&sock_addr, sizeof(SOCKADDR_IN));
	if (ret != NO_ERROR)
	{
		printf("bind error %d", GetLastError());
		return;
	}
	ret =listen(s, SOMAXCONN);
	if (ret != NO_ERROR)
	{
		printf("listen error %d", GetLastError());
		return;
	}
	while (1)
	{
		PLISTENINFO listen_info = new LISTENINFO;
		int addrLen = sizeof(listen_info->addr_in);
		listen_info->s = accept(s, (sockaddr *)&listen_info->addr_in, &addrLen);
		if (listen_info->s != INVALID_SOCKET)
		{
			DWORD threadId = 0;
			HANDLE hd = CreateThread(NULL, NULL, ProcessConn, listen_info, 0, &threadId);
			CloseHandle(hd);
		}
	}
	WSACleanup();
}