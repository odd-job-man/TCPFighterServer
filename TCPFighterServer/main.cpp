#include "Network.h"
#include "Contents.h"
#include <windows.h>
#pragma comment(lib,"Winmm.lib")

constexpr int TICK_PER_FRAME = 20;
constexpr int FRAME_PER_SECONDS = (1000) / TICK_PER_FRAME;


int main()
{
	__int64 a = __rdtsc();
	if (!NetworkInitAndListen())
	{
		return 0;
	}

	timeBeginPeriod(1);
	int time;
	int oldFrameTick = timeGetTime();
	int seconds = oldFrameTick;
	int fpsCounter = 0;
	while (true)
	{
		NetworkProc();
		deleteDisconnected();
		time = timeGetTime();
		if (time - oldFrameTick >= TICK_PER_FRAME)
		{
			Update();
			oldFrameTick += TICK_PER_FRAME;
		}
	}
	timeEndPeriod(1);
}