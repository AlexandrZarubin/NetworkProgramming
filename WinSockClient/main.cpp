#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // !WIN32_LEAN_AND_MEAN

#include<Windows.h>									// Основной заголовок Windows API
#include<WinSock2.h>								// Заголовок для WinSock 2.0
#include<WS2TCPip.h>								// Дополнения к TCP/IP (getaddrinfo)
#include<iphlpapi.h>								// Работа с IP
#include<stdio.h>									// Стандартный ввод/вывод C
#include<iostream>
#include "FormatLastError.h"

using namespace std;

#pragma comment(lib,"Ws2_32.lib")					// Линкуем библиотеку WinSock
#pragma comment(lib, "FormatLastError.lib")

#define DEFAULT_PORT "27015"						// Задаём порт по умолчанию (строкой)
#define DEFAULT_BUFFER_LENGTH 1500

//void PrintLastError(const string& context);
void main()
{
	setlocale(LC_ALL, "rus");
	cout << "WinSock CLient" << endl;
	//1) Инициализация WinSock:
	WSADATA wsaData;																// Структура для данных о WinSock
	INT iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);								// Инициализируем WinSock версии 2.2
	if (iResult)																	// Если произошла ошибка
	{
		cout << "WSAStartup() failed with code " << iResult << endl;
		return;
	}
	//2)Создаем ClientSocket:-Определяем IP-address Сервера
	addrinfo* result = NULL;														// Указатель на результат getaddrinfo
	addrinfo hints;																	// Структура с настройками подключения

	ZeroMemory(&hints, sizeof(hints));												// Обнуляем структуру hints
	hints.ai_family = AF_INET;			//INET	-	TCP/IPv4						// Используем IPv4
	hints.ai_socktype = SOCK_STREAM;												// Потоковый сокет (TCP)
	hints.ai_protocol = IPPROTO_TCP;												// Протокол TCP

	//3)Определяем IP-address Сервера
	iResult = getaddrinfo("127.0.0.1",DEFAULT_PORT,&hints,&result);					// Получаем адрес сервера (localhost:27015)
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
	//char ipStr[INET_ADDRSTRLEN]; // Буфер для IP строки
	//inet_ntop(AF_INET, &(sockaddr_ipv4->sin_addr), ipStr, sizeof(ipStr));
	//cout << "Server IP: " << ipStr << endl;

	// 4) Создание сокета
	SOCKET ConnectSocket = socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol); // Создаём сокет
	if (ConnectSocket == INVALID_SOCKET)												  // Проверяем на ошибку
	{

		cout << "socket() failed: " << WSAGetLastError() << endl;						  // Сообщение об ошибке
		PrintLastError("socket() failed:");
		freeaddrinfo(result);															  // Освобождаем память под result
		WSACleanup();																	  // Завершаем WinSock
		return;
	}

	// 5) Подключение к серверу
	iResult = connect(ConnectSocket, result->ai_addr, result->ai_addrlen);				  // Пытаемся подключиться к серверу
	if (iResult == SOCKET_ERROR)														  // Если ошибка
	{	
		
		cout << "connect() failed: " << WSAGetLastError() << endl;						  // Выводим код ошибки
		PrintLastError("connect() failed");
		closesocket(ConnectSocket);														  // Закрываем сокет
		freeaddrinfo(result);															  // Освобождаем память
		WSACleanup();																	  // Завершаем WinSock
		return;
	}
	
	// 6) Отправка сообщения серверу
	//const char* sendbuf = "Hello from client!";  // Буфер с данными, которые хотим отправить
	//iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0); // Отправляем данные на сервер
	//if (iResult == SOCKET_ERROR)        // Проверяем на ошибку
	//{
	//	cout << "send() failed: " << WSAGetLastError() << endl;
	//	closesocket(ConnectSocket);
	//	freeaddrinfo(result);
	//	WSACleanup();
	//	return;
	//}

	// 6) Отправка сообщения серверу
	CHAR sendbuffer[] = "Hello Server,I am client";
	CHAR recvbuffer[DEFAULT_BUFFER_LENGTH] = {};
	do
	{
	iResult = send(ConnectSocket, sendbuffer, (int)strlen(sendbuffer), 0); // Отправляем данные на сервер
	if (iResult == SOCKET_ERROR)        // Проверяем на ошибку
	{
		cout << "send() failed: " << WSAGetLastError() << endl;
		PrintLastError("Error send");
		closesocket(ConnectSocket);
		freeaddrinfo(result);
		WSACleanup();
		return;
	}
	//// 7) Приём ответа от сервера (опционально)
	//char recvbuf[512];                 // Буфер для получения данных
	//int recvbuflen = 512;              // Размер буфера
	//iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0); // Получаем данные
	//if (iResult > 0)
	//	cout << "Ответ от сервера: " << string(recvbuf, iResult) << endl; // Если получено что-то — выводим
	//else if (iResult == 0)
	//	cout << "Соединение закрыто сервером." << endl;   // Если 0 — соединение закрыто
	//else
	//	cout << "recv() failed: " << WSAGetLastError() << endl; // Если ошибка
	
	//iResult = shutdown(ConnectSocket, SD_SEND);
	//if (iResult == SOCKET_ERROR) PrintLastError("shutdown");

		iResult = recv(ConnectSocket, recvbuffer, DEFAULT_BUFFER_LENGTH, 0);
		if (iResult > 0)cout << "Ответ от сервера: " << iResult << ", Message " << recvbuffer << endl;

		else if (iResult == 0) cout << "Соединение закрыто сервером." << endl;

		else //cout << "recvbuffer() failed: " << WSAGetLastError() << endl;
			PrintLastError("recvbuffer() failed:");
	cout << "Введите сообщение: "; 
	SetConsoleCP(1251);
	cin.getline(sendbuffer,DEFAULT_BUFFER_LENGTH);
	SetConsoleCP(856);
	} while (iResult > 0);
	

	iResult = shutdown(ConnectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) PrintLastError("shutdown");

	// 8) Завершение работы: закрываем сокет и очищаем ресурсы
	closesocket(ConnectSocket);         // Закрываем сокет
	freeaddrinfo(result);               // Освобождаем память, выделенную getaddrinfo
	WSACleanup();                       // Завершаем работу WinSock
}
/*void PrintLastError(const string& context)
{
	DWORD errorCode = WSAGetLastError();      // Получаем код последней ошибки
	LPSTR errorMsg = NULL;
	printf("error%i:%s", errorCode,errorMsg );
	FormatMessage(								// ANSI-версия функции
		FORMAT_MESSAGE_ALLOCATE_BUFFER |        // Автоматически выделить память под сообщение
		FORMAT_MESSAGE_FROM_SYSTEM |            // Получить текст из системных сообщений Windows
		FORMAT_MESSAGE_IGNORE_INSERTS,          // Игнорировать плейсхолдеры типа %1
		NULL,									// Нет пользовательского источника сообщений
		errorCode,                              // Код ошибки, который нужно расшифровать
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),// Язык по умолчанию (не 0!)
		(LPSTR)&errorMsg,                       // Указатель на строку с сообщением
		0,                                      // Максимальная длина (0 — т.к. мы выделяем память)
		NULL                                    // Нет дополнительных параметров
	);

	if (errorMsg) {
		cout << context << " (" << errorCode << "): " << errorMsg << endl;
		LocalFree(errorMsg); // Освобождаем память
	}
	else {
		cout << context << " (" << errorCode << "): Unknown error." << endl;
	}
}*/