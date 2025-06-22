#ifndef WIN32_LEAN_AND_MEAN // Убираем редко‑используемые заголовки из <Windows.h>
#define WIN32_LEAN_AND_MEAN
#endif


#include <Windows.h>          // WinAPI: CreateThread, HeapAlloc, сокеты
#include <WinSock2.h>         // Базовый WinSock 2.0
#include <WS2tcpip.h>         // Расширения TCP/IP: getaddrinfo, inet_ntop, getpeername
#include <stdio.h>            // printf / scanf
#include <iostream>           // std::cout / std::cerr
#include "FormatLastError.h" // Пользовательская функция печати ошибок WinAPI

using namespace std;          // Чтобы не писать std:: перед cout

#pragma comment(lib, "Ws2_32.lib")          // Линкуем ws2_32.lib
#pragma comment(lib, "FormatLastError.lib") // Линкуем PrintLastError.lib


#define DEFAULT_PORT           "27015" // Порт, который будет слушать сервер
#define DEFAULT_BUFFER_LENGTH  1500     // Размер буфера приёма/отправки

SOCKET g_listenSock = INVALID_SOCKET;

bool      InitWinSock();                                            // (1) WSAStartup
addrinfo* ResolveAddress(const char* port);                         // (2) getaddrinfo
SOCKET    CreateListenSocket(addrinfo* addr);                       // (3) socket + bind + listen
SOCKET    AcceptClient(SOCKET listenSock, SOCKADDR_IN& outAddr);    // (4) accept + адрес
DWORD WINAPI ClientSession(LPVOID param);                           // (5) поток‑обработчик клиента
void      PrintClientInfo(const SOCKADDR_IN& a);                    // (6) вывод IP/порт клиента

void      EnsureCapacity(HANDLE*& arr, SIZE_T& cap, SIZE_T need);   // Расширяем массив
SIZE_T    SweepFinishedThreads(HANDLE* arr, SIZE_T& count);
void      CloseAll(HANDLE* arr, SIZE_T count);                      // Закрываем все HANDLE

BOOL WINAPI ConsoleCtrl(DWORD ctrl)
{
    if (   ctrl == CTRL_C_EVENT
        || ctrl == CTRL_BREAK_EVENT
        || ctrl == CTRL_CLOSE_EVENT)
    {
        closesocket(g_listenSock);          // разбудит accept()
        return TRUE;                        // сигнал обработан
    }
    return FALSE;
}

void main()
{
    setlocale(LC_ALL, "rus");                                      
    cout << "WinSockSereverModule" << endl;

    if (!InitWinSock()) return;                                     // (1) WSAStartup

    addrinfo* addr = ResolveAddress(DEFAULT_PORT);                  // (2) Получаем список адресов для прослушивания
    if (!addr) { WSACleanup(); return; }

    SOCKET listenSock = CreateListenSocket(addr);                   // (3) Создаём прослушивающий сокет
    freeaddrinfo(addr);
    if (listenSock == INVALID_SOCKET) 
    {
        WSACleanup();
        return; 
    }

    g_listenSock = listenSock;
    SetConsoleCtrlHandler(ConsoleCtrl, TRUE);                       // регистрируем хэндлер

    cout << "Сервер запущен на порту " << DEFAULT_PORT << ". Ожидаем клиентов…\n";

    HANDLE* hThreads = nullptr;      // Массив HANDLE потоков
    SIZE_T  threadCount = 0;         // Занятых элементов
    SIZE_T  capacity = 0;            // Выделенных элементов

    // ----------------------------- Цикл accept --------------------------
   
    
    do
    {
        SOCKADDR_IN clientAddr{};                                   // Структура под адрес клиента (заполним в AcceptClient)
        SOCKET clientSock = AcceptClient(listenSock, clientAddr);   // (4) Принимаем входящее соединение
        if (clientSock == INVALID_SOCKET)
        {
            INT err = WSAGetLastError();
            if (err == WSAENOTSOCK || err == WSAEINVAL)             // listen-сокет уже закрыт
                break;                                              // выходим из цикла
            else
                continue;                                           // иная ошибка — ждём дальше
        }
        HANDLE hThread = CreateThread(nullptr, 0, ClientSession, (LPVOID)clientSock, 0, nullptr);    // Создаём поток‑обработчик клиента
        if (!hThread)
        {
            PrintLastError("CreateThread failed");
            closesocket(clientSock);
            continue;
        }

        EnsureCapacity(hThreads, capacity, threadCount + 2);        // Держим +1 запас
        hThreads[threadCount++] = hThread;                          // Сохраняем дескриптор

        SIZE_T freed = SweepFinishedThreads(hThreads, threadCount);
        if (freed)
        {
            printf("Sweep: freed %zu slot(s), busy %zu / %zu\n",freed, threadCount, capacity);
            fflush(stdout);
        }
    } while (true);

    // Корректное завершение (недостижимо при бесконечном цикле)
    if (threadCount)
    {
    WaitForMultipleObjects((DWORD)threadCount, hThreads, TRUE, INFINITE);   // Ожидаем завершения всех клиентских потоков
    printf("threadCount hThreads\n"); fflush(stdout);
    CloseAll(hThreads, threadCount);                                        // Закрываем дескрипторы потоков
    }
   
    if (hThreads)
    {
    printf("hThreads HeapFree\n"); fflush(stdout);
    HeapFree(GetProcessHeap(), 0, hThreads);                                // Освобождаем память массива
    }

    closesocket(listenSock);
    printf("exit44 loop\n"); fflush(stdout);
    WSACleanup();
}

// --------------------------- Реализация --------------------------------

bool InitWinSock()
{
    WSADATA wsaData{};
    if (WSAStartup(MAKEWORD(2, 2), &wsaData))
    {
        PrintLastError("WSAStartup failed");
        return false;
    }
    return true;
}

addrinfo* ResolveAddress(const char* port)
{
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    addrinfo* result = NULL;
    if (getaddrinfo(NULL, port, &hints, &result))
    {
        PrintLastError("getaddrinfo failed");
        return NULL;
    }
    return result;
}

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

    if (listen(s, SOMAXCONN) == SOCKET_ERROR)
    {
        PrintLastError("listen failed"); 
        closesocket(s);
        return INVALID_SOCKET;
    }

    return s;
}

SOCKET AcceptClient(SOCKET listenSock, SOCKADDR_IN& outAddr)
{
    int len = sizeof(outAddr);
    SOCKET client = accept(listenSock, (sockaddr*)&outAddr, &len);
    if (client == INVALID_SOCKET) PrintLastError("accept failed");
    return client;
}

void PrintClientInfo(const SOCKADDR_IN& addr)
{
    char ipStr[INET_ADDRSTRLEN] = {};
    inet_ntop(AF_INET, &addr.sin_addr, ipStr, INET_ADDRSTRLEN);
    cout << "Клиент: " << ipStr << ':' << ntohs(addr.sin_port) << "\n";
}

// -------------------- Поток‑обработчик клиента -------------------------
DWORD WINAPI ClientSession(LPVOID param)
{
    SOCKET clientSock = (SOCKET)param;

    SOCKADDR_IN peer{}; 
    int len = sizeof(peer);
    getpeername(clientSock, (sockaddr*)&peer, &len);
    PrintClientInfo(peer);

    char buf[DEFAULT_BUFFER_LENGTH] = {};
    int rec = recv(clientSock, buf, DEFAULT_BUFFER_LENGTH, 0);
    if (rec > 0)
    {
        cout << "Получено (" << rec << "): " << string(buf, rec) << "\n";
        const char* resp = "Hello Client, I am Server!";
        send(clientSock, resp, (INT)strlen(resp), 0);
    }

    shutdown(clientSock, SD_SEND);
    closesocket(clientSock);
    printf("Client disconnected  (TID=%lu)\n", GetCurrentThreadId());
    fflush(stdout);
    return 0;
}

// ----------------- Управление динамическим HANDLE‑массивом -------------
void EnsureCapacity(HANDLE*& arr, SIZE_T& cap, SIZE_T need)                     // Расширяем массив до «need» элементов
{
    if (need <= cap) return;                                           // Ёмкости хватает — выходим

    SIZE_T newCap = cap ? cap * 2 : 2;                                 // Если массив уже есть — удваиваем; иначе начинаем с 2
    while (newCap < need) newCap *= 2;                                 // Увеличиваем, пока не покроем need

    HANDLE* newArr = NULL;
    SIZE_T bytes = newCap * sizeof(HANDLE);

    if (arr == NULL)                                                // Первая аллокация — используем HeapAlloc
    {
        newArr = (HANDLE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, bytes);
        printf("EnsureCapacity: HeapAlloc  %zu -> %zu bytes (cap=%zu)\n",need, bytes, newCap);
    }
    else                                                               // Уже есть память — расширяем HeapReAlloc
    {
        newArr = (HANDLE*)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, arr, bytes);
        printf("EnsureCapacity: HeapReAlloc %zu -> %zu bytes (cap=%zu)\n",cap, bytes, newCap);
    }
    fflush(stdout);
    if (!newArr)                                                       // Проверяем, удалось ли выделить/расширить
    {
        std::cerr << "HeapAlloc/HeapReAlloc failed: insufficient memory";
            ExitProcess(1);                                                // Без памяти продолжать нет смысла
    }

    arr = newArr;                                                      // Обновляем указатель и ёмкость
    cap = newCap;
}

SIZE_T SweepFinishedThreads(HANDLE* arr, SIZE_T& count)
{
    SIZE_T freed = 0;
    for (SIZE_T i = 0; i < count; )
    {
        if (WaitForSingleObject(arr[i], 0) == WAIT_OBJECT_0)
        {
            CloseHandle(arr[i]);                    // закрываем дескриптор
            arr[i] = arr[count - 1];                // переносим последний
            arr[count - 1] = nullptr;
            --count;
            ++freed;
        }
        else ++i;
    }
    return freed;
}

void CloseAll(HANDLE* arr, SIZE_T count)
{
    if (!arr) return;
    for (SIZE_T i = 0; i < count; ++i)
    {
        if (arr[i])
        {
            printf("CloseAll: idx=%zu  handle=%p\n",i,arr[i]);                              
            fflush(stdout);
            CloseHandle(arr[i]);                        // Закрываем каждый дескриптор
        }
    }
}
