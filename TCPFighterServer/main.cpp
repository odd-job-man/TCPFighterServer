#include "Network.h"
#include <windows.h>
#pragma comment(lib,"Winmm.lib")
constexpr int TICK_PER_FRAME = 20;


int main()
{
	if (!NetworkInitAndListen())
	{
		return 0;
	}

	timeBeginPeriod(1);
	int time;
	int oldFrameTick = timeGetTime();
	while (true)
	{
		NetworkProc();
		deleteDisconnected();
		time = timeGetTime();
		if (time - oldFrameTick < TICK_PER_FRAME)
		{
			//Sleep(TICK_PER_FRAME - (time - oldFrameTick));
		}
		oldFrameTick += TICK_PER_FRAME;
	}
	timeEndPeriod(1);
}