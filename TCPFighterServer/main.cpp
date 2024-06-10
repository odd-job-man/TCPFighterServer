#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include "Contents.h"
#include "Network.h"
#include <windows.h>
#pragma comment(lib,"Winmm.lib")

#define new new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
constexpr int TICK_PER_FRAME = 20;
constexpr int FRAME_PER_SECONDS = (1000) / TICK_PER_FRAME;


int main()
{
	timeBeginPeriod(1);
	if (!NetworkInitAndListen())
	{
		return 0;
	}
	int time;
	int oldFrameTick = timeGetTime();
	int seconds = oldFrameTick;
	int fpsCounter = 0;
	int isShutDown = false;
	while (!isShutDown)
	{
		NetworkProc();
		time = timeGetTime();
		if (time - oldFrameTick >= TICK_PER_FRAME)
		{
			Update();
			oldFrameTick += TICK_PER_FRAME;
		}
		if (GetAsyncKeyState(0x35) & 0x8001)
		{
			isShutDown = true;
		}
	}
	ClearSessionInfo();
	timeEndPeriod(1);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	_CrtDumpMemoryLeaks();
	return 0;
}