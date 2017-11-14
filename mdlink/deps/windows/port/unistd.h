#ifndef _UNISTD_H
#define _UNISTD_H

#ifndef _WIN32
#error "Only for Windows!"
#endif

//__declspec(dllimport)
//void __stdcall Sleep(unsigned long dwMilliseconds);

#define _WINSOCKAPI_
#include <Windows.h>

#include <assert.h>

static int usleep(int usec)
{
    assert(usec < 1000000);
    Sleep(usec / 1000 + 1);
    return 0;
}

static int sleep(int seconds)
{
	Sleep(seconds * 1000);
	return 0;
}

#endif
