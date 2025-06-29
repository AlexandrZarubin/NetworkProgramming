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

// ------------------------- Константы -------------------------
#define DEFAULT_PORT "27015"                                 // Порт, который будет слушать сервер
#define DEFAULT_BUFFER_LENGTH 1500                            // Размер буфера под данные




void Clienthandler(SOCKET client_socket);
CONST INT MAX_CONNECTIONS = { 5 };							// Максимальное количество одновременно подключенных клиентов
SOCKET sockets[MAX_CONNECTIONS] = {};						// Массив клиентских сокетов
HANDLE hThreads[MAX_CONNECTIONS] = {};						// Массив дескрипторов потоков
DWORD dwThreadIDs[MAX_CONNECTIONS] = {};					// Массив ID потоков

CRITICAL_SECTION cs;										// Критическая секция для синхронизации доступа к сокетам
void BroadcastMessage(SOCKET sender, const char* data, int len);

INT nextClientID = 1;										// ID, который присваивается следующему клиенту
int clientIDs[MAX_CONNECTIONS] = {};						// Массив ID клиентов
void main()
{
	setlocale(LC_ALL, "rus");

	// 1) Иниуиализация WinSock:
	WSADATA wsaData;																// Структура для данных о WinSock
	INT iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);								// Инициализируем WinSock версии 2.2
	if (iResult!=0)																	// Если произошла ошибка
	{
		PrintLastError("No init");
		return;
	}
	InitializeCriticalSection(&cs);													// Инициализация критической секции

	// 2) Провереяем, не занят ли нужный нам порт 
	addrinfo* result = NULL;														// Указатель на результат getaddrinfo
	addrinfo hints;																	// Структура с настройками подключения
	ZeroMemory(&hints, sizeof(hints));												// Обнуляем структуру hints
	hints.ai_family = AF_INET;			//INET	-	TCP/IPv4						// Используем IPv4
	hints.ai_socktype = SOCK_STREAM;												// Потоковый сокет (TCP)
	hints.ai_protocol = IPPROTO_TCP;												// Протокол TCP
	hints.ai_flags = AI_PASSIVE;													// Подключение к локальному IP

	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0)
	{
		PrintLastError("getaddrinfo failed");
		WSACleanup();
		return;
	}

	// 3) Создаём слушающий сокет
	SOCKET ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET)
	{
		PrintLastError("socket() failed");
		freeaddrinfo(result);
		WSACleanup();
		return;
	}

	// 4) Привязываем сокет к IP и порту
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR)
	{
		PrintLastError("bind failed");
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return;
	}
	freeaddrinfo(result); // больше не нужен
	
	// 5) Переводим сокет в режим прослушивания
	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR)
	{
		PrintLastError("listen failed");
		closesocket(ListenSocket);
		WSACleanup();
		return;
	}

	cout << "Ожидание подключения клиента...\n";

	// 6) Принимаем клиента
	
	//sockaddr_in clientAddr = {};             // Структура для хранения адреса клиента
	//int clientAddrSize = sizeof(clientAddr); // Размер структуры

	//SOCKET ClientSocket = accept(ListenSocket, (sockaddr*)&clientAddr, &clientAddrSize);
	//if (ClientSocket == INVALID_SOCKET)
	//{
	//	PrintLastError("accept failed");
	//	closesocket(ListenSocket);
	//	WSACleanup();
	//	return;
	//}
	
	INT clientCount = 0;

	while (true) 
	{
		SOCKET ClientSocket = accept(ListenSocket, NULL, NULL);
		if (ClientSocket == INVALID_SOCKET) 
		{
			PrintLastError("accept failed");
			continue;
		}

		EnterCriticalSection(&cs);											// Потокобезопасная часть
		if (clientCount < MAX_CONNECTIONS) 
		{
			sockets[clientCount++] = ClientSocket;							// Сохраняем сокет
			clientIDs[clientCount] = nextClientID++;						// Присваиваем ID

			cout << "Клиент подключён. Всего: " << clientCount << "\n";
			CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Clienthandler, (LPVOID)ClientSocket, 0, NULL);
		}
		else 
		{
			cout << "Лимит клиентов исчерпан.\n";
			closesocket(ClientSocket);
		}
		LeaveCriticalSection(&cs);
	}

	// unreachable, но на всякий случай
	closesocket(ListenSocket);
	DeleteCriticalSection(&cs);
	WSACleanup();
}

void Clienthandler(SOCKET client_socket)          
{
	INT iResult = 0;
	char recvbuf[DEFAULT_BUFFER_LENGTH] = {};
	// ===== Отправка клиенту его ID =====
	char helloMsg[64];
	int clientID = 0;
	
	EnterCriticalSection(&cs);
	for (int i = 0; i < MAX_CONNECTIONS; ++i) 
	{
		if (sockets[i] == client_socket) 
		{
			clientID = clientIDs[i];                    // Получаем ID клиента
			break;
		}
	}
	LeaveCriticalSection(&cs);

	sprintf_s(helloMsg, sizeof(helloMsg), "[SERVER]: Вы подключены как Клиент %d", clientID);
	send(client_socket, helloMsg, (int)strlen(helloMsg), 0); // Отправляем ID клиенту
	do
	{
		ZeroMemory(recvbuf, DEFAULT_BUFFER_LENGTH);
		iResult = recv(client_socket, recvbuf, DEFAULT_BUFFER_LENGTH, 0);
		if (iResult > 0)
		{
			cout << "Получено (" << iResult << " байт): " << string(recvbuf, iResult) << endl;

			// Формируем сообщение с ID отправителя
			char message[DEFAULT_BUFFER_LENGTH + 64] = {};
			sprintf_s(message, sizeof(message), "[Клиент %d]: %.*s", clientID, iResult, recvbuf);

			BroadcastMessage(client_socket, message, (int)strlen(message));           // Рассылаем другим клиентам
			if (iResult == SOCKET_ERROR)
			{
				PrintLastError("send failed");
				break;
			}
		}
		else if (iResult == 0)
		{
			cout << "Соединение закрыто клиентом.\n";
		}
		else
		{
			PrintLastError("recv failed");
			break;
		}

	} while (iResult > 0);

	EnterCriticalSection(&cs);
	for (int i = 0; i < MAX_CONNECTIONS; ++i) 
	{
		if (sockets[i] == client_socket) 
		{
			closesocket(sockets[i]);              // Закрываем сокет
			sockets[i] = INVALID_SOCKET;          // Помечаем как свободный
			clientIDs[i] = 0;
			break;
		}
	}
	LeaveCriticalSection(&cs);

	shutdown(client_socket, SD_SEND);
	closesocket(client_socket);
}
void BroadcastMessage(SOCKET sender, const char* data, int len)
{
	EnterCriticalSection(&cs);                // Блокируем доступ к массиву сокетов
	for (int i = 0; i < MAX_CONNECTIONS; ++i)
	{
		if (sockets[i] != INVALID_SOCKET && sockets[i] != sender) 
		{
			send(sockets[i], data, len, 0);   // Отправляем сообщение клиенту
		}
	}
	LeaveCriticalSection(&cs);                // Разблокируем доступ
}
