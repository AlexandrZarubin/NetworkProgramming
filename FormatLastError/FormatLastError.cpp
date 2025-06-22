#include "FormatLastError.h"
#include <Windows.h>
#include <string>
#include <iostream>     // ��� std::cout
using std::string;


void PrintLastError(const string& context)
{
	DWORD errorCode = WSAGetLastError();      // �������� ��� ��������� ������
	LPSTR errorMsg = NULL;
	//printf("error%i:%s", errorCode, errorMsg);
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
		std::cout << context << " (" << errorCode << "): " << errorMsg << std::endl;
		LocalFree(errorMsg); // ����������� ������
	}
	else {
		std::cout << context << " (" << errorCode << "): Unknown error." << std::endl;
	}
}