#define _CRT_SECURE_NO_WARNINGS
#include<Windows.h>
#include"resource.h"
#include<CommCTRL.h>
#include<iostream>
#include<bitset>
#pragma comment(lib, "comctl32.lib")

#define BUFFER_SIZE 1024

BOOL CALLBACK DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

VOID PrintInfo(HWND hwnd);
CHAR* IPtoString(DWORD dwIPaddress, CHAR szIPaddress[]);
INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nCmdShow)
{
	//INITCOMMONCONTROLSEX icc;
	//icc.dwSize = sizeof(icc);
	//icc.dwICC = ICC_INTERNET_CLASSES;
	//InitCommonControlsEx(&icc);
	DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_DIALOG_MAIN), NULL, (DLGPROC)DlgProc, 0);
	return 0;
}

BOOL CALLBACK DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{

	case WM_INITDIALOG:
	{
		HWND hPrefix = GetDlgItem(hwnd, IDC_SPIN_PREFIX);
		SendMessage(hPrefix, UDM_SETRANGE,0,MAKELPARAM( 32, 1));
		AllocConsole();
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
			if (FIRST_IPADDRESS(dwIPaddress) < 128)SendMessage(hEditPrefix, WM_SETTEXT, 0, (LPARAM)"8");//SendMessage(hIPmask, IPM_SETADDRESS, 0, 0xFF000000);
			else if (FIRST_IPADDRESS(dwIPaddress) < 192)SendMessage(hEditPrefix, WM_SETTEXT, 0, (LPARAM)"16");//SendMessage(hIPmask, IPM_SETADDRESS, 0, 0xFFFF0000);
			else if (FIRST_IPADDRESS(dwIPaddress) < 224)SendMessage(hEditPrefix, WM_SETTEXT, 0, (LPARAM)"24");//SendMessage(hIPmask, IPM_SETADDRESS, 0, 0xFFFFFF00);
		}
		break;
		case IDC_SPIN_PREFIX:
		{
			std::cout << "WM_NOTIFY:IDC_SPIN_PREFIX:" << std::endl;

			DWORD dwPrefix = ((NMUPDOWN*)lParam)->iPos;
			INT iDelta = ((NMUPDOWN*)lParam)->iDelta;
			dwPrefix += iDelta;
			//https://learn.microsoft.com/en-us/windows/win32/controls/udn-deltapos
			//https://learn.microsoft.com/en-us/windows/win32/controls/udn-deltapos
			std::cout << dwPrefix << std::endl;
			std::cout << iDelta << std::endl;
			DWORD dwIPmask = ~(0xFFFFFFFF >> dwPrefix);
			//system("PAUSE");
			SendMessage(hIPmask, IPM_SETADDRESS, 0, dwIPmask);
		}
		break;
		case IDC_EDIT_PREFIX:
		{
			CHAR sz_prefix[3] = {};
			SendMessage(hEditPrefix, WM_GETTEXT, 3, (LPARAM)sz_prefix);
			DWORD dwPrefix = atoi(sz_prefix);
			DWORD dwIPmask = ~(0xFFFFFFFF >> dwPrefix);
			SendMessage(hIPmask, IPM_SETADDRESS, 0, dwIPmask);
			PrintInfo(hwnd);
		}
		break;
		case IDOK:
			break;
		case IDCANCEL:
			EndDialog(hwnd, 0);
			break;
		}
	}
	break;
	case WM_NOTIFY:
	{
		std::cout << "WM_NOTIFY:";
		std::cout << "wParam:" << wParam << std::endl;
		HWND hEditPrefix = GetDlgItem(hwnd, IDC_EDIT_PREFIX);
		HWND hIPmask = GetDlgItem(hwnd, IDC_IPMASK);
		DWORD dwIPmask = 0;
		switch (((NMHDR*)lParam)->idFrom)
			//switch (LOWORD(wParam))
		{
		case IDC_IPMASK:
		{
			SendMessage(hIPmask, IPM_GETADDRESS, 0, (LPARAM)&dwIPmask);
			DWORD dwIPprefix = 0;
			for (DWORD iMask = dwIPmask; iMask & 0x80000000; dwIPprefix++)iMask <<= 1;
			CHAR sz_prefix[3];
			sprintf(sz_prefix, "%i", dwIPprefix);
			SendMessage(hEditPrefix, WM_SETTEXT, 0, (LPARAM)sz_prefix);
			//PrintInfo(hwnd);
		}
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

VOID PrintInfo(HWND hwnd)
{
	HWND hIPaddress = GetDlgItem(hwnd, IDC_IPADDRESS);
	HWND hIPmask = GetDlgItem(hwnd, IDC_IPMASK);
	HWND hStaticInfo = GetDlgItem(hwnd, IDC_STATIC_INFO);
	DWORD dwIPaddress = 0;
	DWORD dwIPmask = 0;
	SendMessage(hIPaddress, IPM_GETADDRESS, 0, (LPARAM)&dwIPaddress);
	SendMessage(hIPmask, IPM_GETADDRESS, 0, (LPARAM)&dwIPmask);
	DWORD dwNetworkAddress = dwIPaddress & dwIPmask;
	DWORD dwBroadcastAddress = dwIPaddress | ~dwIPmask;
	DWORD dwNumberOfAdresses = ~dwIPmask + 1;
	DWORD dwNumberOfHost = ~dwIPmask - 1;

	CHAR szNetorkAddress[16] = {};
	CHAR szBroadcastAddress[16] = {};
	CHAR sz_info[BUFFER_SIZE] = {};
	sprintf
	(
		sz_info,
		"Адресс сети:\t\t\t%s;\nШироковещательный адрес:\t%s;\nКоличество IP-адресов:\t%i;\nКоличество узлов:\t\t%i",
		IPtoString(dwNetworkAddress,szNetorkAddress),
		IPtoString(dwBroadcastAddress,szBroadcastAddress),
		dwNumberOfAdresses,
		dwNumberOfHost
	);
	SendMessage(hStaticInfo, WM_SETTEXT, 0, (LPARAM)sz_info);
	
}

CHAR* IPtoString(DWORD dwIPaddress, CHAR szIPaddress[])
{
	sprintf(
		szIPaddress,
		"%i.%i.%i.%i",
		FIRST_IPADDRESS(dwIPaddress),
		SECOND_IPADDRESS(dwIPaddress),
		THIRD_IPADDRESS(dwIPaddress),
		FOURTH_IPADDRESS(dwIPaddress)
	);
	return szIPaddress;
}