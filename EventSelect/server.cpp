#define  _WINSOCK_DEPRECATED_NO_WARNINGS
#include<WinSock2.h>
#include<Windows.h>
#include<stdio.h>

#pragma comment(lib,"ws2_32.lib")
#define LISTEN_PORT 8899
#define MAX_EVENT 64
#define BUFLEN 1024
int totalClient = 0;
SOCKET sClients[MAX_EVENT];
WSAEVENT eClients[MAX_EVENT];
CRITICAL_SECTION criLock;

DWORD WINAPI ProcessConn(LPVOID)
{
	while (TRUE)
	{
		DWORD index = WSAWaitForMultipleEvents(totalClient, eClients, FALSE, WSA_INFINITE, FALSE);
		for (index; index < totalClient;++index)
		{
			WSANETWORKEVENTS netWorkEvent;
			WSAEnumNetworkEvents(sClients[index], eClients[index], &netWorkEvent);
			if (netWorkEvent.lNetworkEvents & FD_CLOSE)
			{
				closesocket(sClients[index]);
				WSACloseEvent(eClients[index]);
				EnterCriticalSection(&criLock);
				for (int j = index; j < totalClient-1; ++j)  //前移socket
				{
					sClients[j] = sClients[j + 1];
					eClients[j] = eClients[j + 1];
				}
				totalClient--;
				index--;
				LeaveCriticalSection(&criLock);
			}
			else if (netWorkEvent.lNetworkEvents & FD_READ)
			{
				char buff[BUFLEN] = {0};
				recv(sClients[index], buff, BUFLEN, 0);
				printf("[recv]:%s\n", buff);
				char sendStr[] = "hello event select";
				send(sClients[index], sendStr, sizeof(sendStr), 0);
			}
		}
	}
	WSACleanup();
	return 0;
}

void main()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	
	SOCKET s;
	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SOCKADDR_IN sock_addr;
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(LISTEN_PORT);
	sock_addr.sin_addr.S_un.S_addr = htonl(ADDR_ANY);

	bind(s, (sockaddr *)&sock_addr, sizeof(sock_addr));
	listen(s, 128);
	//创建事件
	WSAEVENT wsaEvent = WSACreateEvent();
	WSAEventSelect(s, wsaEvent, FD_ACCEPT | FD_CLOSE);
	InitializeCriticalSection(&criLock);
	DWORD threadId = 0;
	CreateThread(NULL, NULL, ProcessConn, NULL, 0, &threadId);
	while (TRUE)
	{
		WSAWaitForMultipleEvents(1, &wsaEvent, TRUE, WSA_INFINITE, FALSE);
		WSANETWORKEVENTS netWorkEvent;
		WSAEnumNetworkEvents(s, wsaEvent, &netWorkEvent);
		if (netWorkEvent.lNetworkEvents & FD_ACCEPT)
		{
			if (totalClient == MAX_EVENT)
			{
				printf("max event count\n");
				continue;
			}
			SOCKADDR_IN sock_in;
			int len = sizeof(sock_in);
			SOCKET sc = accept(s, (sockaddr*)&sock_in, &len);
			char *ip = inet_ntoa(sock_in.sin_addr);
			printf("[hostname]%s connected\n",ip);
			WSAEVENT sEvent = WSACreateEvent();
			EnterCriticalSection(&criLock);
			sClients[totalClient] = sc;
			WSAEventSelect(sc, sEvent, FD_READ | FD_CLOSE);
			eClients[totalClient++] = sEvent;
			LeaveCriticalSection(&criLock);
		}
		else
		{
			break;
		}
	}
	closesocket(s);
	WSACloseEvent(wsaEvent);
	WSACleanup();
	DeleteCriticalSection(&criLock);
}