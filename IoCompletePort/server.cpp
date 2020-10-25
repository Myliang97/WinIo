#define  _WINSOCK_DEPRECATED_NO_WARNINGS
#include<WinSock2.h>
#include<windows.h>
#include<stdio.h>
#pragma comment(lib,"ws2_32.lib")
#define LISTEN_PORT 8899
#define BUFLEN 1024

typedef struct _DataInfo
{
	WSAOVERLAPPED overLapped;
	WSABUF buf;
	CHAR data[BUFLEN];
	DWORD dataLen;
	DWORD flag;
	SOCKET s;
}DataInfo,*PDataInfo;

DWORD WINAPI WorkProc(LPVOID lparameter)
{
	HANDLE iocomPort = (HANDLE)lparameter;
	DWORD dataLen=0;
	DWORD sKey = 0;
	PDataInfo dataInfo = NULL;
	while (TRUE)
	{
		if (!GetQueuedCompletionStatus(iocomPort, &dataLen,&sKey, (LPOVERLAPPED *)&dataInfo, INFINITE))
		{
			printf("get io complete status err:%d\n", GetLastError());
			return 0;
		}
		if (dataLen == 0 || dataLen == 0xFFFFFFFF)
		{
			printf("close socket %d\n", dataInfo->s);
			closesocket(dataInfo->s);
			if (dataInfo)
				delete dataInfo;
		}
		else
		{
			printf("[recv]: data:%s len:%d \n", dataInfo->buf.buf, dataLen);
			CHAR bufsend[1024] = "hello iocompleteport";
			send(dataInfo->s, bufsend, strlen(bufsend), 0);
			dataInfo->buf.buf = dataInfo->data;
			dataInfo->buf.len = BUFLEN;
			memset(dataInfo->data, 0, sizeof(dataInfo->data));
			memset(&dataInfo->overLapped, 0, sizeof(dataInfo->overLapped));  //开始处理下一个异步操作
			WSARecv(dataInfo->s, &dataInfo->buf, 1, &dataInfo->dataLen, &dataInfo->flag, &dataInfo->overLapped, NULL);
		}
	}
	return 0;
}

void main()
{
	WSADATA wsadata;
	WSAStartup(MAKEWORD(2, 2), &wsadata);
	
	HANDLE ioCmPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	for (int i = 0; i < sysInfo.dwNumberOfProcessors * 2; ++i)
	{
		CreateThread(NULL, NULL, WorkProc, ioCmPort, 0, NULL);
	}
	SOCKET s = WSASocketA(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN server_addr;
	server_addr.sin_addr.S_un.S_addr = htonl(ADDR_ANY);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(LISTEN_PORT);
	
	bind(s, (sockaddr *)&server_addr, sizeof(server_addr));
	listen(s, 64);
	while (TRUE)
	{
		SOCKADDR_IN addr_in;
		int len = sizeof(addr_in);
		PDataInfo pack = new DataInfo;
		pack->buf.buf = pack->data;
		pack->buf.len = BUFLEN;
		memset(pack->data, 0, sizeof(pack->data));
		pack->s = WSAAccept(s, (sockaddr *)&addr_in, &len, NULL, NULL);
		printf("[hostname]: %s connected\n", inet_ntoa(addr_in.sin_addr));
		CreateIoCompletionPort((HANDLE)pack->s, ioCmPort,NULL, 0);
		if (SOCKET_ERROR == WSARecv(pack->s, &pack->buf, 1, &pack->dataLen, &pack->flag, &pack->overLapped, NULL))
		{
			if (WSAGetLastError() != ERROR_IO_PENDING)
			{
				printf("last err:%d\n", WSAGetLastError());
			}
		}
	}
	PostQueuedCompletionStatus(ioCmPort, 0xFFFFFFFF, 0, NULL);
	CloseHandle(ioCmPort);
	closesocket(s);
	WSACleanup();
}
