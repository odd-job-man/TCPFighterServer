#include "Contents.h"
#include "Network.h"
#include <map>
#include <set>
#define IN
#define OUT
#define LOG

constexpr int ERROR_RANGE = 50;
constexpr int MOVE_UNIT_X = 3;
constexpr int MOVE_UNIT_Y = 2;
extern int g_IdToAlloc;
std::map<int, ClientInfo*> g_clientMap;
#define new new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )

extern std::set<int> g_disconnected_id_set;
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

void ClearClientInfo()
{
	for (auto iter = g_clientMap.begin(); iter != g_clientMap.end();)
	{
		delete iter->second;
		iter = g_clientMap.erase(iter);
	}
	g_clientMap.clear();
}
inline bool isDeletedSession(const int id)
{
	return g_disconnected_id_set.find(id) != g_disconnected_id_set.end();
}
inline void HandleMoving(IN ClientInfo* pCI, IN int moveDir)
{
	bool isNoX = false;
	bool isNoY = false;

	if (pCI->x - MOVE_UNIT_X <= dfRANGE_MOVE_LEFT || pCI->x + MOVE_UNIT_X >= dfRANGE_MOVE_RIGHT)
	{
		isNoX = true;
	}

	if (pCI->y - MOVE_UNIT_Y <= dfRANGE_MOVE_TOP || pCI->y + MOVE_UNIT_Y >= dfRANGE_MOVE_BOTTOM)
	{
		isNoY = true;
	}

	switch (moveDir)
	{
	case dfPACKET_MOVE_DIR_LL:
		if (!isNoX)
		{
			pCI->x -= MOVE_UNIT_X;
#ifdef LOG
			wprintf(L"Session ID : %d #\tDIR : LL\tMove Simulate X : %d, Y : %d\n", pCI->ID, pCI->x, pCI->y);
#endif
		}
		break;
	case dfPACKET_MOVE_DIR_LU:
		if (!isNoX && !isNoY)
		{
			pCI->x -= MOVE_UNIT_X;
			pCI->y -= MOVE_UNIT_Y;
#ifdef LOG
			wprintf(L"Session ID : %d #\tDIR : LU\tMove Simulate X : %d, Y : %d\n", pCI->ID, pCI->x, pCI->y);
#endif
		}
		break;
	case dfPACKET_MOVE_DIR_UU:
		if (!isNoY)
		{
			pCI->y -= MOVE_UNIT_Y;
#ifdef LOG
			wprintf(L"Session ID : %d #\tDIR : UU\tMove Simulate X : %d, Y : %d\n", pCI->ID, pCI->x, pCI->y);
#endif
		}
		break;
	case dfPACKET_MOVE_DIR_RU:
		if (!isNoX && !isNoY)
		{
			pCI->x += MOVE_UNIT_X;
			pCI->y -= MOVE_UNIT_Y;
#ifdef LOG
			wprintf(L"Session ID : %d #\tDIR : RU\tMove Simulate X : %d, Y : %d\n", pCI->ID, pCI->x, pCI->y);
#endif
			break;
		}
	case dfPACKET_MOVE_DIR_RR:
		if (!isNoX)
		{
			pCI->x += MOVE_UNIT_X;
#ifdef LOG
			wprintf(L"Session ID : %d #\tDIR : RR\tMove Simulate X : %d, Y : %d\n", pCI->ID, pCI->x, pCI->y);
#endif
		}
		break;
	case dfPACKET_MOVE_DIR_RD:
		if (!isNoX && !isNoY)
		{
			pCI->x += MOVE_UNIT_X;
			pCI->y += MOVE_UNIT_Y;
#ifdef LOG
			wprintf(L"Session ID : %d #\tDIR : RD\tMove Simulate X : %d, Y : %d\n", pCI->ID, pCI->x, pCI->y);
#endif
		}
		break;
	case dfPACKET_MOVE_DIR_DD:
		if (!isNoY)
		{
			pCI->y += MOVE_UNIT_Y;
#ifdef LOG
			wprintf(L"Session ID : %d #\tDIR : DD\tMove Simulate X : %d, Y : %d\n", pCI->ID, pCI->x, pCI->y);
#endif
		}
		break;
	case dfPACKET_MOVE_DIR_LD:
		if (!isNoX && !isNoY)
		{
			pCI->x -= MOVE_UNIT_X;
			pCI->y += MOVE_UNIT_Y;
#ifdef LOG
			wprintf(L"Session ID : %d #\tDIR : LD\tMove Simulate X : %d, Y : %d\n", pCI->ID, pCI->x, pCI->y);
#endif
		}
		break;
	default:
		__debugbreak();
		return;
	}
}
inline bool IsCollision_ATTACK1(ClientInfo* pAttackerCI, ClientInfo* pAttackedCI)
{
	int dX = abs(pAttackerCI->x - pAttackedCI->x);
	int dY = abs(pAttackerCI->y - pAttackedCI->y);
	bool isValidX = (dX <= dfATTACK1_RANGE_X) ? true : false;
	bool isValidY = (dY <= dfATTACK1_RANGE_Y) ? true : false;
	return isValidX && isValidY;
}

inline bool IsCollision_ATTACK2(ClientInfo* pAttackerCI, ClientInfo* pAttackedCI)
{
	int dX = abs(pAttackerCI->x - pAttackedCI->x);
	int dY = abs(pAttackerCI->y - pAttackedCI->y);
	bool isValidX = (dX <= dfATTACK2_RANGE_X) ? true : false;
	bool isValidY = (dY <= dfATTACK2_RANGE_Y) ? true : false;
	return isValidX && isValidY;
}

inline bool IsCollision_ATTACK3(ClientInfo* pAttackerCI, ClientInfo* pAttackedCI)
{
	int dX = abs(pAttackerCI->x - pAttackedCI->x);
	int dY = abs(pAttackerCI->y - pAttackedCI->y);
	bool isValidX = (dX <= dfATTACK3_RANGE_X) ? true : false;
	bool isValidY = (dY <= dfATTACK3_RANGE_Y) ? true : false;
	return isValidX && isValidY;
}
void MAKE_PACKET_HEADER(Header* pHeader, BYTE packetSize, BYTE packetType);
void MAKE_PACKET_SC_CREATE_MY_CHARACTER(PACKET_SC_CREATE_MY_CHARACTER* pSCMC, int id);
void MAKE_PACKET_SC_CREATE_OTHER_CHARACTER(PACKET_SC_CREATE_OTHER_CHARARCTER* pPSCOC, const ClientInfo* pCI);
void MAKE_PACKET_SC_DELETE_CHATACTER(PACKET_SC_DELETE_CHARACTER* pPSDC, int id);
void MAKE_PACKET_SC_MOVE_START(PACKET_SC_MOVE_START* pPSMS, int id, char dir, USHORT x, USHORT y);
void MAKE_PACKET_SC_MOVE_STOP(PACKET_SC_MOVE_STOP* pPSMS, int id, char dir, USHORT x, USHORT y);
void MAKE_PACKET_SC_ATTACK1(PACKET_SC_ATTACK1* pPSA1, int id, char dir, USHORT x, USHORT y);
void MAKE_PACKET_SC_ATTACK2(PACKET_SC_ATTACK2* pPSA2, int id, char dir, USHORT x, USHORT y);
void MAKE_PACKET_SC_ATTACK3(PACKET_SC_ATTACK3* pPSA3, int id, char dir, USHORT x, USHORT y);
void MAKE_PACKET_SC_DAMAGE(PACKET_SC_DAMAGE* pPSD, int attackerId, int attackedId, char hpToReduce);
bool PROCESS_PACKET_CS_MOVE_START(int moveId, char* pPacket);
bool PROCESS_PACKET_CS_MOVE_STOP(int stopId, char* pPacket);
bool PROCESS_PACKET_CS_ATTACK1(int attackerId, char* pPacket);
bool PROCESS_PACKET_CS_ATTACK2(int attackerId, char* pPacket);
bool PROCESS_PACKET_CS_ATTACK3(int attackerId, char* pPacket);
bool EnqNewRBForOtherCreate(IN Session* pNewUser);
void EnqPacketBroadCast(IN char* pPacket, IN const size_t packetSize, IN OPTIONAL const int pTargetIdToExcept);

bool PacketProc(int id, BYTE packetType, char* pPacket)
{
	switch (packetType)
	{
	case dfPACKET_CS_MOVE_START:
		return PROCESS_PACKET_CS_MOVE_START(id,pPacket);
	case dfPACKET_CS_MOVE_STOP:
		return PROCESS_PACKET_CS_MOVE_STOP(id, pPacket);
	case dfPACKET_CS_ATTACK1:
		return PROCESS_PACKET_CS_ATTACK1(id, pPacket);
	case dfPACKET_CS_ATTACK2:
		return PROCESS_PACKET_CS_ATTACK2(id, pPacket);
	case dfPACKET_CS_ATTACK3:
		return PROCESS_PACKET_CS_ATTACK3(id, pPacket);
	default:
		return false;
	}
}

bool PROCESS_PACKET_CS_MOVE_START(int moveId, char* pPacket)
{
	PACKET_CS_MOVE_START* pPCMS = (PACKET_CS_MOVE_START*)pPacket;
	ClientInfo* pCI = g_clientMap.find(moveId)->second;
	bool isNotValidPos = abs(pCI->x - pPCMS->x) > ERROR_RANGE || abs(pCI->y - pPCMS->y) > ERROR_RANGE;
	if (isNotValidPos)
	{
#ifdef LOG
		wprintf(L"Session ID : %d\t PROCESS_PACKET_CS_MOVE_START#\tMOVE_RANGE_ERROR - disconnected\n", moveId);
		wprintf(L"Server X : %d, Y : %d, Client X : %d, Y : %d\n", pCI->x, pCI->y, pPCMS->x, pPCMS->y);
#endif
		disconnect(moveId);
		return false;
	}
#ifdef LOG
	wprintf(L"Session ID : %d\tPACKET_CS_MOVE_START PROCESSING #\tMoveDir : %d\tX:%d\tY:%d\n", moveId, pPCMS->moveDir, pPCMS->x, pPCMS->y);
#endif
	switch (pPCMS->moveDir)
	{
	case dfPACKET_MOVE_DIR_RR:
	case dfPACKET_MOVE_DIR_RU:
	case dfPACKET_MOVE_DIR_RD:
		pCI->viewDir = dfPACKET_MOVE_DIR_RR;
		break;
	case dfPACKET_MOVE_DIR_LU:
	case dfPACKET_MOVE_DIR_LL:
	case dfPACKET_MOVE_DIR_LD:
		pCI->viewDir = dfPACKET_MOVE_DIR_LL;
		break;
	default:
		//__debugbreak();
		break;
	}
	pCI->x = pPCMS->x;
	pCI->y = pPCMS->y;
	pCI->action = pPCMS->moveDir;
	PACKET_SC_MOVE_START PSMS;
	MAKE_PACKET_SC_MOVE_START(&PSMS, pCI->ID, pPCMS->moveDir, pPCMS->x, pPCMS->y);
#ifdef LOG
	wprintf(L"Session ID : %d -> Other\tPAKCET_SC_MOVE_START\tBroadCast\n", pCI->ID);
#endif
	EnqPacketBroadCast((char*)&PSMS, sizeof(PSMS), moveId);
	return true;
}
bool PROCESS_PACKET_CS_MOVE_STOP(int stopId, char* pPacket)
{
	PACKET_CS_MOVE_STOP* pPCMS = (PACKET_CS_MOVE_STOP*)pPacket;
	ClientInfo* pCI = g_clientMap.find(stopId)->second;
	bool isNotValidPos = abs(pCI->x - pPCMS->x) > ERROR_RANGE || abs(pCI->y - pPCMS->y) > ERROR_RANGE;
	if (isNotValidPos)
	{
#ifdef LOG
		wprintf(L"Session ID : %d\t PROCESS_PACKET_CS_MOVE_STOP#\tMOVE_RANGE_ERROR - disconnected\n", stopId);
		wprintf(L"Server X : %d, Y : %d, Client X : %d, Y : %d\n", pCI->x, pCI->y, pPCMS->x, pPCMS->y);
#endif
		disconnect(stopId);
		return false;
	}
#ifdef LOG
	wprintf(L"Session ID : %d\tPACKET_CS_MOVE_STOP PROCESSING #\tMoveDir : %d\tX:%d\tY:%d\n", stopId, pPCMS->viewDir, pPCMS->x, pPCMS->y);
#endif
	pCI->viewDir = pPCMS->viewDir;
	pCI->x = pPCMS->x;
	pCI->y = pPCMS->y;
	pCI->action = dfPACKET_MOVE_DIR_NOMOVE;
	PACKET_SC_MOVE_STOP PSMS;
	MAKE_PACKET_SC_MOVE_STOP(&PSMS, pCI->ID, pCI->viewDir, pCI->x, pCI->y);

#ifdef LOG
	wprintf(L"Session ID : %d -> Other\tPAKCET_SC_MOVE_STOP BroadCast\n", pCI->ID);
#endif
	EnqPacketBroadCast((char*)&PSMS, sizeof(PSMS), stopId);
	return true;
}

bool PROCESS_PACKET_CS_ATTACK1(int attackerId,char* pPacket)
{
#ifdef LOG
	wprintf(L"Session ID : %d\t PROCESS_PACKET_CS_ATTACK1#\n", attackerId);
#endif
	PACKET_CS_ATTACK1* pPCA1 = (PACKET_CS_ATTACK1*)(pPacket);
	ClientInfo* pCI = g_clientMap.find(attackerId)->second;
	if (!(pCI->x == pPCA1->x && pCI->y == pPCA1->y))
		__debugbreak();

	PACKET_SC_ATTACK1 PSA1;
	MAKE_PACKET_SC_ATTACK1(&PSA1, attackerId, pCI->viewDir, pCI->x, pCI->y);
#ifdef LOG
	wprintf(L"Session ID : %d -> Other \t PROCESS_PACKET_SC_ATTACK1 BroadCast#\n", attackerId);
#endif
	EnqPacketBroadCast((char*)&PSA1, sizeof(PSA1), attackerId);
	for (auto iter = g_clientMap.begin(); iter != g_clientMap.end(); ++iter)
	{
		ClientInfo* pAttackedCI = iter->second;
		if (pAttackedCI == pCI)
			continue;

		if (IsCollision_ATTACK1(pCI, pAttackedCI))
		{
			PACKET_SC_DAMAGE PSD;
			pAttackedCI->hp -= 10;
			MAKE_PACKET_SC_DAMAGE(&PSD, pCI->ID, pAttackedCI->ID,pAttackedCI->hp);
#ifdef LOG
			wprintf(L"Session ID : %d -> Attacked %d\t PROCESS_PACKET_SC_DAMAGE#\n", attackerId,pAttackedCI->ID);
#endif
			EnqPacketBroadCast((char*)&PSD, sizeof(PSD), -1);
		}
	}
	return true;
}

bool PROCESS_PACKET_CS_ATTACK2(int attackerId,char* pPacket)
{
#ifdef LOG
	wprintf(L"Session ID : %d\t PROCESS_PACKET_CS_ATTACK2#\n", attackerId);
#endif
	PACKET_CS_ATTACK2* pPCA2 = (PACKET_CS_ATTACK2*)(pPacket);
	ClientInfo* pCI = g_clientMap.find(attackerId)->second;
	if (!(pCI->x == pPCA2->x && pCI->y == pPCA2->y))
		__debugbreak();

	PACKET_SC_ATTACK2 PSA2;
	MAKE_PACKET_SC_ATTACK2(&PSA2, attackerId, pCI->viewDir, pCI->x, pCI->y);
#ifdef LOG
	wprintf(L"Session ID : %d -> Other \t PROCESS_PACKET_SC_ATTACK2 BroadCast#\n", attackerId);
#endif
	EnqPacketBroadCast((char*)&PSA2, sizeof(PSA2), attackerId);
	for (auto iter = g_clientMap.begin(); iter != g_clientMap.end(); ++iter)
	{
		ClientInfo* pAttackedCI = iter->second;
		if (pAttackedCI == pCI)
			continue;

		if (IsCollision_ATTACK2(pCI, pAttackedCI))
		{
			PACKET_SC_DAMAGE PSD;
			pAttackedCI->hp -= 10;
			MAKE_PACKET_SC_DAMAGE(&PSD, pCI->ID, pAttackedCI->ID, pAttackedCI->hp);
#ifdef LOG
			wprintf(L"Session ID : %d -> Attacked %d\t PROCESS_PACKET_SC_DAMAGE#\n", attackerId,pAttackedCI->ID);
#endif
			EnqPacketBroadCast((char*)&PSD, sizeof(PSD), -1);
		}
	}
	return true;
}

bool PROCESS_PACKET_CS_ATTACK3(int attackerId,char* pPacket)
{
#ifdef LOG
	wprintf(L"Session ID : %d\t PROCESS_PACKET_CS_ATTACK3#\n", attackerId);
#endif
	PACKET_CS_ATTACK3* pPCA3 = (PACKET_CS_ATTACK3*)(pPacket);
	ClientInfo* pCI = g_clientMap.find(attackerId)->second;
	if (!(pCI->x == pPCA3->x && pCI->y == pPCA3->y))
		__debugbreak();

	PACKET_SC_ATTACK3 PSA3;
	MAKE_PACKET_SC_ATTACK3(&PSA3, attackerId, pCI->viewDir, pCI->x, pCI->y);
#ifdef LOG
	wprintf(L"Session ID : %d -> Other \t PROCESS_PACKET_SC_ATTACK2 BroadCast#\n", attackerId);
#endif
	EnqPacketBroadCast((char*)&PSA3, sizeof(PSA3), attackerId);
	for (auto iter = g_clientMap.begin(); iter != g_clientMap.end(); ++iter)
	{
		ClientInfo* pAttackedCI = iter->second;
		if (pAttackedCI == pCI)
			continue;

		if (IsCollision_ATTACK2(pCI, pAttackedCI))
		{
			PACKET_SC_DAMAGE PSD;
			pAttackedCI->hp -= 20;
			MAKE_PACKET_SC_DAMAGE(&PSD, pCI->ID, pAttackedCI->ID, pAttackedCI->hp);
#ifdef LOG
			wprintf(L"Session ID : %d -> Attacked %d\t PROCESS_PACKET_SC_DAMAGE#\n", attackerId, pAttackedCI->ID);
#endif
			EnqPacketBroadCast((char*)&PSD, sizeof(PSD), -1);
		}
	}
	return true;
}


void Update()
{
	for (auto iter = g_clientMap.begin(); iter != g_clientMap.end(); ++iter)
	{
		ClientInfo* pCI = iter->second;
		if (isDeletedSession(pCI->ID))
		{
			continue;
		}

		if (pCI->action != dfPACKET_MOVE_DIR_NOMOVE)
		{
			HandleMoving(pCI, pCI->action);
		}

		if (pCI->hp <= 0)
		{
#ifdef LOG
			wprintf(L"Session ID : %d Die\t HP : %d\n", pCI->ID, pCI->hp);
#endif
			disconnect(pCI->ID);
		}
	}
	deleteDisconnected();
}

// 패킷 만드는 함수
void MAKE_PACKET_HEADER(Header* pHeader, BYTE packetSize, BYTE packetType)
{
	pHeader->byCode = 0x89;
	pHeader->bySize = packetSize;
	pHeader->byType = packetType;

}
void MAKE_PACKET_SC_CREATE_MY_CHARACTER(PACKET_SC_CREATE_MY_CHARACTER* pSCMC, int id)
{
	MAKE_PACKET_HEADER(&pSCMC->header, sizeof(PACKET_SC_CREATE_MY_CHARACTER) - sizeof(pSCMC->header), dfPACKET_SC_CREATE_MY_CHARACTER);
	pSCMC->id = id;
	pSCMC->direction = dfPACKET_MOVE_DIR_LL;
	pSCMC->x = INIT_POS_X;
	pSCMC->y = INIT_POS_Y;
	pSCMC->HP = INIT_HP;
}

void MAKE_PACKET_SC_CREATE_OTHER_CHARACTER(PACKET_SC_CREATE_OTHER_CHARARCTER* pPSCOC,const ClientInfo* pCI)
{
	MAKE_PACKET_HEADER(&pPSCOC->header, sizeof(*pPSCOC) - sizeof(pPSCOC->header), dfPACKET_SC_CREATE_OTHER_CHARACTER);
	pPSCOC->id = pCI->ID;
	pPSCOC->direction = pCI->viewDir;
	pPSCOC->x = pCI->x;
	pPSCOC->y = pCI->y;
	pPSCOC->HP = pCI->hp;
}

void MAKE_PACKET_SC_DELETE_CHATACTER(PACKET_SC_DELETE_CHARACTER* pPSDC, int id)
{
	MAKE_PACKET_HEADER(&pPSDC->header, sizeof(*pPSDC) - sizeof(pPSDC->header), dfPACKET_SC_DELETE_CHARACTER);
	pPSDC->id = id;
}

void MAKE_PACKET_SC_MOVE_START(PACKET_SC_MOVE_START* pPSMS, int id, char dir, USHORT x, USHORT y)
{
	MAKE_PACKET_HEADER(&pPSMS->header, sizeof(*pPSMS) - sizeof(pPSMS->header), dfPACKET_SC_MOVE_START);
	pPSMS->id = id;
	pPSMS->direction = dir;
	pPSMS->x = x;
	pPSMS->y = y;
}

void MAKE_PACKET_SC_MOVE_STOP(PACKET_SC_MOVE_STOP* pPSMS, int id, char dir, USHORT x, USHORT y)
{
	MAKE_PACKET_HEADER(&pPSMS->header, sizeof(PACKET_SC_MOVE_STOP) - sizeof(pPSMS->header), dfPACKET_SC_MOVE_STOP);
	pPSMS->id = id;
	pPSMS->direction = dir;
	pPSMS->x = x;
	pPSMS->y = y;
}


void MAKE_PACKET_SC_ATTACK1(PACKET_SC_ATTACK1* pPSA1, int id, char viewDir, USHORT x, USHORT y)
{
	MAKE_PACKET_HEADER(&pPSA1->header, sizeof(*pPSA1) - sizeof(pPSA1->header), dfPACKET_SC_ATTACK1);
	pPSA1->id = id;
	pPSA1->viewDir = viewDir;
	pPSA1->x = x;
	pPSA1->y = y;
}

void MAKE_PACKET_SC_ATTACK2(PACKET_SC_ATTACK2* pPSA2, int id, char viewDir, USHORT x, USHORT y)
{
	MAKE_PACKET_HEADER(&pPSA2->header, sizeof(*pPSA2) - sizeof(pPSA2->header), dfPACKET_SC_ATTACK2);
	pPSA2->id = id;
	pPSA2->viewDir = viewDir;
	pPSA2->x = x;
	pPSA2->y = y;
}

void MAKE_PACKET_SC_ATTACK3(PACKET_SC_ATTACK3* pPSA3, int id, char viewDir, USHORT x, USHORT y)
{
	MAKE_PACKET_HEADER(&pPSA3->header, sizeof(*pPSA3) - sizeof(pPSA3->header), dfPACKET_SC_ATTACK3);
	pPSA3->id = id;
	pPSA3->viewDir = viewDir;
	pPSA3->x = x;
	pPSA3->y = y;
}

void MAKE_PACKET_SC_DAMAGE(PACKET_SC_DAMAGE* pPSD, int attackerId, int attackedId, char hpToReduce)
{
	MAKE_PACKET_HEADER(&pPSD->header, sizeof(*pPSD) - sizeof(pPSD->header), dfPACKET_SC_DAMAGE);
	pPSD->attackerId = attackerId;
	pPSD->attackedId = attackedId;
	pPSD->hpToReduce = hpToReduce;
}

// 클라이언트의 접속을 끊는 함수들
void disconnect(int id)
{
	g_disconnected_id_set.insert(id);
#ifdef LOG
	wprintf(L"disconnect Session id : %d#\n", id);
#endif 
}

