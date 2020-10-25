#define  _WINSOCK_DEPRECATED_NO_WARNINGS
#include<WinSock2.h>
#include<stdio.h>
#include<windows.h>
#pragma comment(lib,"ws2_32.lib")

#define LISTEN_PORT 8899
#define BUFLEN 1024

typedef struct _DataPack
{
	WSAOVERLAPPED overLap;
	WSABUF buf;
	CHAR data[BUFLEN];
	DWORD dataLen;
	DWORD flag;
	SOCKET s;
}DataPack,*PDataPack;
SOCKET sClient;
HANDLE newEvent;
CRITICAL_SECTION criSection;

void CALLBACK COMPLETION_ROUTINE(
	IN DWORD dwError,
	IN DWORD cbTransferred,
	IN LPWSAOVERLAPPED lpOverlapped,
	IN DWORD dwFlags
	)
{
	PDataPack pack = (PDataPack)lpOverlapped;
	if (dwError != 0 || cbTransferred ==0 )
	{
		closesocket(pack->s);
		delete pack;
		return;
	}
	printf("[recv]:data:%s len:%d\n", pack->data, cbTransferred);
	CHAR bufsend[1024] = "hello overlapped";
	send(pack->s, bufsend, strlen(bufsend), 0);
	pack->buf.len = BUFLEN;   
	pack->buf.buf = pack->data;
	memset(pack->data, 0, sizeof(pack->data));
	memset(&pack->overLap, 0, sizeof(pack->overLap));  //开始处理下一个异步操作
	WSARecv(pack->s, &pack->buf, 1, &pack->dataLen, &pack->flag, &pack->overLap, COMPLETION_ROUTINE);
}

DWORD WINAPI WorkProc(LPVOID)
{
	while (TRUE)
	{
		PDataPack pack = new DataPack;
		pack->buf.len = BUFLEN;
		pack->buf.buf = pack->data;
		memset(pack->data, 0, sizeof(pack->data));
		WSAWaitForMultipleEvents(1, &newEvent, FALSE, INFINITE, TRUE);  //设置当完成例程完成时，事件也被激活
		pack->s = sClient;
		WSARecv(pack->s, &pack->buf, 1, &pack->dataLen, &pack->flag, &pack->overLap, COMPLETION_ROUTINE);
		EnterCriticalSection(&criSection);
		WSAResetEvent(newEvent);
		LeaveCriticalSection(&criSection);
	}
	return 0;
}

void main()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SOCKADDR_IN server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(LISTEN_PORT);
	server_addr.sin_addr.S_un.S_addr = htonl(ADDR_ANY);

	bind(s, (sockaddr *)&server_addr, sizeof(server_addr));
	listen(s, 64);
	sClient = INVALID_SOCKET;
	newEvent = WSACreateEvent();
	InitializeCriticalSection(&criSection);
	CreateThread(NULL, NULL, WorkProc, NULL, 0, NULL);
	while (TRUE)
	{
		SOCKADDR_IN client_addr;
		int len = sizeof(client_addr);
		sClient = accept(s, (sockaddr*)&client_addr,&len);
		printf("[hostname]:%s connected \n", inet_ntoa(client_addr.sin_addr));
		EnterCriticalSection(&criSection);
		WSASetEvent(newEvent);
		LeaveCriticalSection(&criSection);
	}
	DeleteCriticalSection(&criSection);
	WSACleanup();
}