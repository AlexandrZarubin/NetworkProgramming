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
	
	// 6) �������� ��������� �������
	//const char* sendbuf = "Hello from client!";  // ����� � �������, ������� ����� ���������
	//iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0); // ���������� ������ �� ������
	//if (iResult == SOCKET_ERROR)        // ��������� �� ������
	//{
	//	cout << "send() failed: " << WSAGetLastError() << endl;
	//	closesocket(ConnectSocket);
	//	freeaddrinfo(result);
	//	WSACleanup();
	//	return;
	//}

	// 6) �������� ��������� �������
	CHAR sendbuffer[] = "Hello Server,I am client";
	CHAR recvbuffer[DEFAULT_BUFFER_LENGTH] = {};
	do
	{
	iResult = send(ConnectSocket, sendbuffer, (int)strlen(sendbuffer), 0); // ���������� ������ �� ������
	if (iResult == SOCKET_ERROR)        // ��������� �� ������
	{
		cout << "send() failed: " << WSAGetLastError() << endl;
		PrintLastError("Error send");
		closesocket(ConnectSocket);
		freeaddrinfo(result);
		WSACleanup();
		return;
	}
	//// 7) ���� ������ �� ������� (�����������)
	//char recvbuf[512];                 // ����� ��� ��������� ������
	//int recvbuflen = 512;              // ������ ������
	//iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0); // �������� ������
	//if (iResult > 0)
	//	cout << "����� �� �������: " << string(recvbuf, iResult) << endl; // ���� �������� ���-�� � �������
	//else if (iResult == 0)
	//	cout << "���������� ������� ��������." << endl;   // ���� 0 � ���������� �������
	//else
	//	cout << "recv() failed: " << WSAGetLastError() << endl; // ���� ������
	
	//iResult = shutdown(ConnectSocket, SD_SEND);
	//if (iResult == SOCKET_ERROR) PrintLastError("shutdown");

		iResult = recv(ConnectSocket, recvbuffer, DEFAULT_BUFFER_LENGTH, 0);
		if (iResult > 0)cout << "����� �� �������: " << iResult << ", Message " << recvbuffer << endl;

		else if (iResult == 0) cout << "���������� ������� ��������." << endl;

		else //cout << "recvbuffer() failed: " << WSAGetLastError() << endl;
			PrintLastError("recvbuffer() failed:");
	cout << "������� ���������: "; 
	SetConsoleCP(1251);
	cin.getline(sendbuffer,DEFAULT_BUFFER_LENGTH);
	SetConsoleCP(856);
	} while (iResult > 0);
	

	iResult = shutdown(ConnectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) PrintLastError("shutdown");

	// 8) ���������� ������: ��������� ����� � ������� �������
	closesocket(ConnectSocket);         // ��������� �����
	freeaddrinfo(result);               // ����������� ������, ���������� getaddrinfo
	WSACleanup();                       // ��������� ������ WinSock
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