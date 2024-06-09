#include "ClientInfo.h"
#include "Network.h"
#include <map>
#define IN
#define OUT
#define LOG

extern int g_IdToAlloc;
std::map<int, ClientInfo*> g_clientMap;

void InitClientInfo(OUT ClientInfo* pCI, IN int id)
{
	pCI->ID = id;
	pCI->hp = INIT_HP;
	pCI->x = INIT_POS_X;
	pCI->y = INIT_POS_Y;
	pCI->action = dfPACKET_MOVE_DIR_NOMOVE;
	pCI->viewDir = INIT_DIR;
#ifdef LOG
	wprintf(L"Create Character # SessionID : %d\tX:%d\tY:%d\n", id, INIT_POS_X, INIT_POS_Y);
#endif
}
