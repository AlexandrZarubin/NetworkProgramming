#include "FormatLastError.h"
#include <Windows.h>
#include <string>
#include <iostream>     // Для std::cout
using std::string;


void PrintLastError(const string& context)
{
	DWORD errorCode = WSAGetLastError();      // Получаем код последней ошибки
	LPSTR errorMsg = NULL;
	//printf("error%i:%s", errorCode, errorMsg);
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
		std::cout << context << " (" << errorCode << "): " << errorMsg << std::endl;
		LocalFree(errorMsg); // Освобождаем память
	}
	else {
		std::cout << context << " (" << errorCode << "): Unknown error." << std::endl;
	}
}