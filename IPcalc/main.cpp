#define _CRT_SECURE_NO_WARNINGS
#include<Windows.h>
#include"resource.h"
#include<CommCTRL.h>
#include<iostream>
#include<bitset>
#pragma comment(lib, "comctl32.lib")

BOOL CALLBACK DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


void PrintBinaryMask(DWORD mask) 
{
	printf("dwIPmask bits: ");
	for (int i = 31; i >= 0; i--) 
	{
		printf("%d", (mask >> i) & 1);
		if (i % 8 == 0 && i != 0) printf(" ");
	}
	printf("\n");
}
void UpdateMaskAndPrefix(HWND hwnd);

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nCmdShow)
{
	INITCOMMONCONTROLSEX icc;
	icc.dwSize = sizeof(icc);
	icc.dwICC = ICC_INTERNET_CLASSES;
	InitCommonControlsEx(&icc);
	DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_DIALOG_MAIN), NULL, (DLGPROC)DlgProc, 0);
	return 0;
}

BOOL CALLBACK DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static 	DWORD lastMask = 0xFFFFFFFF;
	switch (uMsg)
	{

	case WM_INITDIALOG:
	{
		HWND hPrefix = GetDlgItem(hwnd, IDC_SPIN_PREFIX);
		SendMessage(hPrefix, UDM_SETRANGE,0,MAKELPARAM( 32, 1));
		AllocConsole();

		SetConsoleCP(CP_UTF8);
		SetConsoleOutputCP(CP_UTF8);

		freopen("CONOUT$", "w", stdout); // привязка stdout к консоли
		std::ios::sync_with_stdio(true);
		std::cout.clear();
		std::cout.flush();
		//std::cout.rdbuf(std::cout.rdbuf());
		printf("WM_INITDIALOG: create console.\n");
		fflush(stdout);
		SetFocus(GetDlgItem(hwnd,IDC_IPADDRESS));
		return TRUE;
	}
		break;
	case WM_NOTIFY:
	{
		LPNMHDR pNmHdr = (LPNMHDR)lParam;
		if (pNmHdr->idFrom == IDC_IPMASK && pNmHdr->code == IPN_FIELDCHANGED)
		{
			UpdateMaskAndPrefix(hwnd);
			return FALSE;
		}
	}
		break;
	case WM_COMMAND:
	{
		HWND hIPaddress = GetDlgItem(hwnd, IDC_IPADDRESS);
		HWND hIPmask = GetDlgItem(hwnd, IDC_IPMASK);
		HWND hEditPrefix = GetDlgItem(hwnd, IDC_EDIT_PREFIX);
		DWORD dwIPaddress = 0;
		DWORD dwIPmask = 0;
		
		switch (LOWORD(wParam))
		{
			case IDC_IPADDRESS:
			{
				SendMessage(hIPaddress, IPM_GETADDRESS, 0, (LPARAM)&dwIPaddress);
				if (FIRST_IPADDRESS(dwIPaddress) < 128)SendMessage(hIPmask, IPM_SETADDRESS, 0, 0xFF000000);
				else if (FIRST_IPADDRESS(dwIPaddress) < 192)SendMessage(hIPmask, IPM_SETADDRESS, 0, 0xFFFF0000);
				else if (FIRST_IPADDRESS(dwIPaddress) < 224)SendMessage(hIPmask, IPM_SETADDRESS, 0, 0xFFFFFF00);
				UpdateMaskAndPrefix(hwnd);

			}
			break;
			case IDC_EDIT_PREFIX:
				if (HIWORD(wParam) == EN_CHANGE)
				{
					int p = GetDlgItemInt(hwnd, IDC_EDIT_PREFIX, 0, 0);
					DWORD m = 0xFFFFFFFF << (32 - p);
					SendDlgItemMessage(hwnd, IDC_IPMASK, IPM_SETADDRESS, 0, m);
				}
				break;
			//case IDC_IPMASK:
			//{
			//	if (HIWORD(wParam) == EN_CHANGE || HIWORD(wParam) == CBN_SELCHANGE)
			//	{
			//		SendMessage(hIPmask, IPM_GETADDRESS, 0, (LPARAM)&dwIPmask);
			//		DWORD prefix = 0;
			//		for (int i = 31; i >= 0; i--)
			//		{
			//			
			//			DWORD shifted = dwIPmask >> i;

			//			// Показываем 32-битный результат после сдвига
			//			std::bitset<32> bits(shifted);
			//			std::cout << "i = " << i << " | shifted = " << bits;

			//			// Показываем младший бит
			//			std::cout << " | bit " << (shifted & 1);

			//			if ((shifted & 1) == 1)
			//			{
			//				prefix++;
			//				std::cout << " -> prefix++ = " << prefix << std::endl;
			//			}
			//			else
			//			{
			//				std::cout << " -> break\n";
			//				break;
			//			}
			//		}
			//		PrintBinaryMask(dwIPmask);
			//		char buf[4] = {};
			//		sprintf(buf, "%d", prefix);
			//		SendMessage(hEditPrefix, WM_SETTEXT, 0, (LPARAM)buf);
			//	}
			//}
			//break;
			case IDOK:
				break;
			case IDCANCEL:
				FreeConsole();
				EndDialog(hwnd, 0);
				break;
		}

	}
		break;
	case WM_CLOSE:
		FreeConsole();
		EndDialog(hwnd, 0);
	}
	return FALSE;
}

void UpdateMaskAndPrefix(HWND hwnd)
{
	HWND hIPmask = GetDlgItem(hwnd, IDC_IPMASK);
	HWND hEditPrefix = GetDlgItem(hwnd, IDC_EDIT_PREFIX);
	DWORD dwIPmask = 0;
	SendMessage(hIPmask, IPM_GETADDRESS, 0, (LPARAM)&dwIPmask);

	printf("=== Маска обновлена ===\n");
	printf("Исходная маска (DWORD): 0x%08X\n", dwIPmask);
	PrintBinaryMask(dwIPmask);

	DWORD prefix = 0;
	for (int i = 31; i >= 0; i--)
	{
		DWORD shifted = dwIPmask >> i;
		std::bitset<32> bits(shifted);
		std::cout << "i = " << i
			<< " | shifted = " << bits
			<< " | bit " << (shifted & 1);

		if ((shifted & 1) == 1)
		{
			prefix++;
			std::cout << " -> prefix++ = " << prefix << std::endl;
		}
		else
		{
			std::cout << " -> break\n";
			break;
		}
	}
	PrintBinaryMask(dwIPmask);

	char buf[4] = {};
	sprintf(buf, "%d", prefix);
	SendMessage(hEditPrefix, WM_SETTEXT, 0, (LPARAM)buf);
}