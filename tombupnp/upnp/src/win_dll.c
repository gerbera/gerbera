// upnpLib.cpp : Defines the entry point for the DLL application.
//

#ifdef WIN32

#include <windows.h>
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
	}
    return TRUE;
}

#endif
