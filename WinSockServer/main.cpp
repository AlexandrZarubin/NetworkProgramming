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

// -------------------- Предварительные прототипы --------------
bool InitWinSock();                                           // (1) Загрузка DLL WinSock (WSAStartup)
addrinfo* ResolveAddress(const char* port);                   // (2) Получаем подходящий адрес для bind()
SOCKET  CreateListenSocket(addrinfo* addr);                   // (3) Создаём TCP‑сокет и привязываем его
SOCKET  AcceptClient(SOCKET listenSock, sockaddr_in& outAddr);// (4) Принимаем клиента, возвращаем его адрес
void    PrintClientInfo(const sockaddr_in& addr);             // (5) Выводим IP/порт клиента
void    HandleClient(SOCKET clientSock);                      // (6) Приём‑отправка данных
void    Cleanup(SOCKET sock);                                 // (7) Корректно завершаем работу
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
	
	/*SOCKET ClientSocket = accept(ListenSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET)
	{
		PrintLastError("accept failed");
		closesocket(ListenSocket);
		WSACleanup();
		return;
	}
	*/


	/* ------------------------показываем IP и порт клиента ---------------- */
	// Вариант А — получаем адрес прямо в accept 
	sockaddr_in clientAddr{};                                  // Структура для IP/порт клиента
	int clientAddrSize = sizeof(clientAddr);
	SOCKET ClientSocket = accept(ListenSocket, (sockaddr*)&clientAddr, &clientAddrSize);
	//SOCKET ClientSocket = accept(ListenSocket, NULL,NULL);
	if (ClientSocket == INVALID_SOCKET)
	{
		PrintLastError("accept failed");
		closesocket(ListenSocket);
		WSACleanup();
		return;
	}

	char ipStr[INET_ADDRSTRLEN] = {};
	inet_ntop(AF_INET, &clientAddr.sin_addr, ipStr, INET_ADDRSTRLEN); // Перевели IP в текстовую форму
	cout << "Клиент подключён (вариант A): IP=" << ipStr << ", порт=" << ntohs(clientAddr.sin_port) << "\n";

	// Вариант B — если по какой‑то причине адрес не сохранили, можно узнать его через getpeername
	sockaddr_in peerAddr{};
	int peerAddrSize = sizeof(peerAddr);
	if (getpeername(ClientSocket, (sockaddr*)&peerAddr, &peerAddrSize) == 0)
	{
		char ipStr2[INET_ADDRSTRLEN] = {};
		inet_ntop(AF_INET, &peerAddr.sin_addr, ipStr2, INET_ADDRSTRLEN);
		cout << "Клиент подключён (вариант B): IP=" << ipStr2 << ", порт=" << ntohs(peerAddr.sin_port) << "\n";
	}
	else
	{
		PrintLastError("getpeername failed");
	}




	cout << "Клиент подключён!\n";

	closesocket(ListenSocket); // больше не нужен


	// 7) Обработка клиента
	char recvbuf[DEFAULT_BUFFER_LENGTH] = {};

	do
	{
		ZeroMemory(recvbuf, DEFAULT_BUFFER_LENGTH);
		iResult = recv(ClientSocket, recvbuf, DEFAULT_BUFFER_LENGTH, 0);
		if (iResult > 0)
		{
			cout << "Получено (" << iResult << " байт): " << string(recvbuf, iResult) << endl;

			 //Ответ клиенту
			const char* sendbuf = "Hello Client, I am Server!";
			iResult = send(ClientSocket, sendbuf, (int)strlen(sendbuf), 0);
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

	// 8) Завершение соединения
	iResult = shutdown(ClientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR)
	{
		PrintLastError("shutdown failed");
	}

	closesocket(ClientSocket);
	WSACleanup();

}

// (1) Загрузка DLL WinSock (WSAStartup)
bool InitWinSock()											
{
	WSADATA wsaData{};
	int res = WSAStartup(MAKEWORD(2, 2), &wsaData);           // Просим версию 2.2
	if (res != 0)
	{
		PrintLastError("WSAStartup failed");
		return false;
	}
	return true;
}

// (2) Получаем подходящий адрес для bind()
addrinfo* ResolveAddress(const char* port)
{
	addrinfo hints{};
	hints.ai_family = AF_INET;									// IPv4
	hints.ai_socktype = SOCK_STREAM;							// TCP
	hints.ai_protocol = IPPROTO_TCP;							// TCP
	hints.ai_flags = AI_PASSIVE;								// Слушать локальный адрес

	addrinfo* result = nullptr;
	int res = getaddrinfo(nullptr, port, &hints, &result);
	if (res != 0)
	{
		PrintLastError("getaddrinfo failed");
		return nullptr;
	}
	return result;
}

// (3) Создаём TCP‑сокет и привязываем его
SOCKET CreateListenSocket(addrinfo* addr)
{
	SOCKET s = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	if (s == INVALID_SOCKET)
	{
		PrintLastError("socket failed");
		return INVALID_SOCKET;
	}

	if (bind(s, addr->ai_addr, static_cast<int>(addr->ai_addrlen)) == SOCKET_ERROR)
	{
		PrintLastError("bind failed");
		closesocket(s);
		return INVALID_SOCKET;
	}

	if (listen(s, SOMAXCONN) == SOCKET_ERROR)                 // Переходим в listen
	{
		PrintLastError("listen failed");
		closesocket(s);
		return INVALID_SOCKET;
	}
	return s;                                                 // Готовый socket в режиме ожидания
}


// (4) Принимаем клиента, возвращаем его адрес
SOCKET AcceptClient(SOCKET listenSock, sockaddr_in& outAddr)
{
	int addrSize = sizeof(outAddr);
	SOCKET client = accept(listenSock, reinterpret_cast<sockaddr*>(&outAddr), &addrSize);
	if (client == INVALID_SOCKET)
	{
		PrintLastError("accept failed");
	}
	return client;
}

// (5) Выводим IP/порт клиента
void PrintClientInfo(const sockaddr_in& addr)
{
	char ipStr[INET_ADDRSTRLEN] = {};
	inet_ntop(AF_INET, &addr.sin_addr, ipStr, INET_ADDRSTRLEN);
	cout << "Клиент подключён: IP=" << ipStr << ", порт=" << ntohs(addr.sin_port) << "\n";
}

// (6) Приём‑отправка данных
void HandleClient(SOCKET clientSock)
{
	char recvBuf[DEFAULT_BUFFER_LENGTH] = {};

	int received = recv(clientSock, recvBuf, DEFAULT_BUFFER_LENGTH, 0);
	if (received > 0)
	{
		cout << "Получено (" << received << " байт): " << string(recvBuf, received) << "\n";

		const char* reply = "Hello Client, I am Server!";
		int sent = send(clientSock, reply, static_cast<int>(strlen(reply)), 0);
		if (sent == SOCKET_ERROR)
		{
			PrintLastError("send failed");
		}
	}
	else if (received == 0)
	{
		cout << "Клиент закрыл соединение.\n";
	}
	else
	{
		PrintLastError("recv failed");
	}

	// Завершаем общение корректно — отправляем FIN
	if (shutdown(clientSock, SD_SEND) == SOCKET_ERROR)
	{
		PrintLastError("shutdown failed");
	}
}

// (7) Корректно завершаем работу
void Cleanup(SOCKET sock)
{
	closesocket(sock);                                        // Закрываем переданный сокет
}
