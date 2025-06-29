#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // !WIN32_LEAN_AND_MEAN

#include<Windows.h>									// �������� ��������� Windows API
#include<WinSock2.h>								// ��������� ��� WinSock 2.0
#include<WS2TCPip.h>								// ���������� � TCP/IP (getaddrinfo)
#include<iphlpapi.h>								// ������ � IP
#include<stdio.h>									// ����������� ����/����� C
#include<iostream>
#include "FormatLastError.h"


using namespace std;

#pragma comment(lib,"Ws2_32.lib")					// ������� ���������� WinSock
#pragma comment(lib, "FormatLastError.lib")

#define DEFAULT_PORT "27015"						// ����� ���� �� ��������� (�������)
#define DEFAULT_BUFFER_LENGTH 1500

DWORD WINAPI ReceiveThread(LPVOID socket_ptr);   // ������� ������ �����
DWORD WINAPI SendThread(LPVOID socket_ptr);      // ������� ������ ��������

//void PrintLastError(const string& context);
void main()
{
	setlocale(LC_ALL, "rus");
	cout << "WinSock CLient" << endl;
	//1) ������������� WinSock:
	WSADATA wsaData;																// ��������� ��� ������ � WinSock
	INT iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);								// �������������� WinSock ������ 2.2
	if (iResult)																	// ���� ��������� ������
	{
		cout << "WSAStartup() failed with code " << iResult << endl;
		return;
	}
	//2)������� ClientSocket:-���������� IP-address �������
	addrinfo* result = NULL;														// ��������� �� ��������� getaddrinfo
	addrinfo hints;																	// ��������� � ����������� �����������

	ZeroMemory(&hints, sizeof(hints));												// �������� ��������� hints
	hints.ai_family = AF_INET;			//INET	-	TCP/IPv4						// ���������� IPv4
	hints.ai_socktype = SOCK_STREAM;												// ��������� ����� (TCP)
	hints.ai_protocol = IPPROTO_TCP;												// �������� TCP

	//3)���������� IP-address �������
	iResult = getaddrinfo("127.0.0.1",DEFAULT_PORT,&hints,&result);					// �������� ����� ������� (localhost:27015)
	if (iResult)
	{
		cout << "getaddressinfo() failed with code" << iResult << endl;
		PrintLastError("getaddrinfo() failed:");
		WSACleanup();
		return;
	}
	//cout << "hints:" << endl;
	//cout << "ai_addr:" << hints.ai_addr->sa_data << endl;
	
	//sockaddr_in* sockaddr_ipv4 = (sockaddr_in*)result->ai_addr;
	//char ipStr[INET_ADDRSTRLEN]; // ����� ��� IP ������
	//inet_ntop(AF_INET, &(sockaddr_ipv4->sin_addr), ipStr, sizeof(ipStr));
	//cout << "Server IP: " << ipStr << endl;

	// 4) �������� ������
	SOCKET ConnectSocket = socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol); // ������ �����
	if (ConnectSocket == INVALID_SOCKET)												  // ��������� �� ������
	{

		cout << "socket() failed: " << WSAGetLastError() << endl;						  // ��������� �� ������
		PrintLastError("socket() failed:");
		freeaddrinfo(result);															  // ����������� ������ ��� result
		WSACleanup();																	  // ��������� WinSock
		return;
	}

	// 5) ����������� � �������
	iResult = connect(ConnectSocket, result->ai_addr, result->ai_addrlen);				  // �������� ������������ � �������
	if (iResult == SOCKET_ERROR)														  // ���� ������
	{	
		
		cout << "connect() failed: " << WSAGetLastError() << endl;						  // ������� ��� ������
		PrintLastError("connect() failed");
		closesocket(ConnectSocket);														  // ��������� �����
		freeaddrinfo(result);															  // ����������� ������
		WSACleanup();																	  // ��������� WinSock
		return;
	}
	
	// ������ ����� ��� ����� ��������� �� �������
	HANDLE hRecvThread = CreateThread(NULL, 0, ReceiveThread, (LPVOID)ConnectSocket, 0, NULL);
	
	// ������ ����� ��� �������� ��������� �������
	HANDLE hSendThread = CreateThread(NULL, 0, SendThread, (LPVOID)ConnectSocket, 0, NULL);


	// ��� ���������� ������ ��������
	WaitForSingleObject(hSendThread, INFINITE);
	// ������������� ��������� ����� �����
	TerminateThread(hRecvThread, 0);

	

	// ��������� ����������� �������
	CloseHandle(hRecvThread);
	CloseHandle(hSendThread);
	closesocket(ConnectSocket);        // ��������� �����
	freeaddrinfo(result);              // ����������� ������ getaddrinfo
	WSACleanup();                      // ��������� ������ � WinSock
}
/*void PrintLastError(const string& context)
{
	DWORD errorCode = WSAGetLastError();      // �������� ��� ��������� ������
	LPSTR errorMsg = NULL;
	printf("error%i:%s", errorCode,errorMsg );
	FormatMessage(								// ANSI-������ �������
		FORMAT_MESSAGE_ALLOCATE_BUFFER |        // ������������� �������� ������ ��� ���������
		FORMAT_MESSAGE_FROM_SYSTEM |            // �������� ����� �� ��������� ��������� Windows
		FORMAT_MESSAGE_IGNORE_INSERTS,          // ������������ ������������ ���� %1
		NULL,									// ��� ����������������� ��������� ���������
		errorCode,                              // ��� ������, ������� ����� ������������
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),// ���� �� ��������� (�� 0!)
		(LPSTR)&errorMsg,                       // ��������� �� ������ � ����������
		0,                                      // ������������ ����� (0 � �.�. �� �������� ������)
		NULL                                    // ��� �������������� ����������
	);

	if (errorMsg) {
		cout << context << " (" << errorCode << "): " << errorMsg << endl;
		LocalFree(errorMsg); // ����������� ������
	}
	else {
		cout << context << " (" << errorCode << "): Unknown error." << endl;
	}
}*/
// ������� ����� ��������� �� �������
DWORD WINAPI ReceiveThread(LPVOID socket_ptr)
{
	SOCKET ConnectSocket = (SOCKET)socket_ptr;         // �������� ��������� � SOCKET
	char recvbuffer[DEFAULT_BUFFER_LENGTH] = {};       // ����� ��� ������
	int iResult;

	do {
		ZeroMemory(recvbuffer, DEFAULT_BUFFER_LENGTH); // ������� �����
		iResult = recv(ConnectSocket, recvbuffer, DEFAULT_BUFFER_LENGTH, 0); // ��������� ������
		if (iResult > 0) {
			// ���� ���� ������ � �������
			cout << "\n[������]: " << string(recvbuffer, iResult) << "\n������� ���������: ";
		}
		else if (iResult == 0) {
			// ������ �������� ����������
			cout << "\n[������ ������ ����������]\n";
			break;
		}
		else {
			// ������ �����
			PrintLastError("recv failed");
			break;
		}
	} while (iResult > 0);

	return 0;
}

// ������� �������� ��������� �� ������
DWORD WINAPI SendThread(LPVOID socket_ptr)
{
	SOCKET ConnectSocket = (SOCKET)socket_ptr;         // �������� ��������� � SOCKET
	char sendbuffer[DEFAULT_BUFFER_LENGTH] = {};       // ����� ��� ��������
	int iResult;

	while (true) {
		cout << "������� ���������: ";                 // ������ �� ����
		SetConsoleCP(1251);                            // ������������� ��������� �����
		cin.getline(sendbuffer, DEFAULT_BUFFER_LENGTH); // ������ ������ �� �������
		SetConsoleCP(866);                             // ���������� ��������� �������

		if (strcmp(sendbuffer, "exit") == 0)           // ���� ������� "exit", �������
			break;

		// �������� ��������� �� ������
		iResult = send(ConnectSocket, sendbuffer, (int)strlen(sendbuffer), 0);
		if (iResult == SOCKET_ERROR) {
			PrintLastError("send failed");             // ������ ��� ��������
			break;
		}
	}

	shutdown(ConnectSocket, SD_SEND);                  // ��������� ��������
	return 0;
}