#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include <map>
#include <queue>
#include <set>
#include "ClientInfo.h"
#include "Network.h"

#define LOG
#pragma comment(lib,"ws2_32.lib")

SOCKET g_listenSock;
std::map<int, Session*>g_sessionMap;
extern std::map<int, ClientInfo*> g_clientMap;
constexpr int SERVERPORT = 5000;
constexpr int MAX_USER_NUM = 30;
constexpr int ERROR_RANGE = 50;
constexpr int MOVE_UNIT_X = 3;
constexpr int MOVE_UNIT_Y = 2;

std::queue<int> id_q;
std::set<int> g_disconnected_id_set;
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
void Update();
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

bool AcceptProc();
void RecvProc(Session* pSession);
void SendProc(Session* pSession);
inline void EnqPacketUnicast(IN const int id,IN char* pPacket,IN const size_t packetSize)
{
	RingBuffer& sendRB = g_sessionMap.find(id)->second->sendBuffer;

	int EnqRet = sendRB.Enqueue(pPacket, packetSize);
	if (EnqRet == 0)
	{
#ifdef LOG
		wprintf(L"Session ID : %d\tsendRingBuffer Full\t", id);
#endif
		disconnect(id);
	}
}
inline bool isDeletedSession(const int id)
{
	return g_disconnected_id_set.find(id) != g_disconnected_id_set.end();
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
			MAKE_PACKET_SC_DAMAGE(&PSD, pCI->ID, pAttackedCI->ID, 10);
			pAttackedCI->hp -= 10;
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
			MAKE_PACKET_SC_DAMAGE(&PSD, pCI->ID, pAttackedCI->ID, 10);
			pAttackedCI->hp -= 10;
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
			MAKE_PACKET_SC_DAMAGE(&PSD, pCI->ID, pAttackedCI->ID, 20);
			pAttackedCI->hp -= 20;
#ifdef LOG
			wprintf(L"Session ID : %d -> Attacked %d\t PROCESS_PACKET_SC_DAMAGE#\n", attackerId, pAttackedCI->ID);
#endif
			EnqPacketBroadCast((char*)&PSD, sizeof(PSD), -1);
		}
	}
	return true;
}


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
		break;
	}
}
bool NetworkInitAndListen()
{
	int bindRet;
	int listenRet;
	int ioctlRet;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		wprintf(L"WSAStartup() func error code : %d\n", WSAGetLastError());
		return false;
	}
#ifdef LOG
	wprintf(L"WSAStartup #\n");
#endif

	g_listenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (g_listenSock == INVALID_SOCKET)
	{
		wprintf(L"socket() func error code : %d\n", WSAGetLastError());
		return false;
	}

	SOCKADDR_IN serverAddr;
	ZeroMemory(&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	serverAddr.sin_port = htons(SERVERPORT);

	bindRet = bind(g_listenSock, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (bindRet == SOCKET_ERROR)
	{
		wprintf(L"bind() func error code : %d\n", WSAGetLastError());
		return false;
	}
#ifdef LOG
	wprintf(L"Bind OK # Port : %d\n", SERVERPORT);
#endif

	listenRet = listen(g_listenSock, SOMAXCONN);
	if (listenRet == SOCKET_ERROR)
	{
		wprintf(L"listen() func error code : %d\n", WSAGetLastError());
		return false;
	}
#ifdef LOG
	wprintf(L"Listen OK #\n");
#endif

	u_long iMode = 1;
	ioctlRet = ioctlsocket(g_listenSock, FIONBIO, &iMode);
	if (ioctlRet != 0)
	{
		wprintf(L"ioctlsocket() func error code : %d\n", WSAGetLastError());
		return false;
	}
	return true;
}

bool NetworkProc()
{
	static bool isFirst = true;
	int selectRet;
	fd_set readSet;
	fd_set writeSet;
	FD_ZERO(&readSet);
	FD_ZERO(&writeSet);
	FD_SET(g_listenSock, &readSet);
	for (auto iter = g_sessionMap.begin(); iter != g_sessionMap.end(); ++iter)
	{
		FD_SET(iter->second->clientSock, &readSet);
		if (iter->second->sendBuffer.GetUseSize() > 0)
		{
			FD_SET(iter->second->clientSock, &writeSet);
		}
	}

	if (isFirst)
	{
		for (int i = 0; i < MAX_USER_NUM; ++i)
		{
			id_q.push(i);
		}
		isFirst = false;
	}

	SOCKADDR_IN clientAddr;
	int addrlen = sizeof(clientAddr);
	timeval tv{ 0,0 };
	selectRet = select(0, &readSet, &writeSet, nullptr, &tv);
	if (selectRet == 0)
	{
		return false;
	}
	if (selectRet == SOCKET_ERROR)
	{
		wprintf(L"select func error code : %d\n", WSAGetLastError());
		return false;
	}
	if (FD_ISSET(g_listenSock, &readSet))
	{
		AcceptProc();
		--selectRet;
	}

	for (auto iter = g_sessionMap.begin(); iter != g_sessionMap.end() && selectRet > 0; ++iter)
	{
		Session* pSession = iter->second;
		if (FD_ISSET(pSession->clientSock, &readSet))
		{
			RecvProc(pSession);
			--selectRet;
		}

		if (FD_ISSET(iter->second->clientSock, &writeSet))
		{
			SendProc(pSession);
			--selectRet;
		}
	}
}

bool AcceptProc()
{
	SOCKET clientSock;
	SOCKADDR_IN clientAddr;
	int addrlen = sizeof(clientAddr);
	clientSock = accept(g_listenSock, (SOCKADDR*)&clientAddr, &addrlen);
	if (clientSock == INVALID_SOCKET)
	{
		wprintf(L"accept func error code : %d\n", WSAGetLastError());
		__debugbreak();
		return false;
	}


	if (g_sessionMap.size() >= MAX_USER_NUM)
	{
		// 연결끊기
		closesocket(clientSock);
		return false;
	}
	int newID = id_q.front();
	id_q.pop();
#ifdef LOG
	WCHAR szIpAddr[MAX_PATH];
	InetNtop(AF_INET, &clientAddr.sin_addr, szIpAddr, _countof(szIpAddr));
	wprintf(L"Connect # ip : %s / SessionId:%d\n", szIpAddr, newID);
#endif 
	Session* pNewSession = new Session{ clientSock,newID };
	g_sessionMap.insert(std::pair<int, Session*>{newID, pNewSession});

	ClientInfo* pNewClientInfo = new ClientInfo;
	InitClientInfo(pNewClientInfo, newID);
	g_clientMap.insert(std::pair<int, ClientInfo*>{newID, pNewClientInfo});

	PACKET_SC_CREATE_MY_CHARACTER PSCMC;
	MAKE_PACKET_SC_CREATE_MY_CHARACTER(&PSCMC, newID);
	int PSCMCEnqRet = pNewSession->sendBuffer.Enqueue((char*)&PSCMC, sizeof(PSCMC));
	if (PSCMCEnqRet == 0)
	{
#ifdef LOG
		wprintf(L"Session ID : %d\tsendRingBuffer Full\t", newID);
#endif
		disconnect(newID);
		return false;
	}
#ifdef LOG
	wprintf(L"Session ID : %d, PACKET_SC_CREATE_MY_CHARACTER sendRB Enqueue Size : %d\n", newID, PSCMCEnqRet);
#endif
	PACKET_SC_CREATE_OTHER_CHARARCTER PSCOC;
	MAKE_PACKET_SC_CREATE_OTHER_CHARACTER(&PSCOC, pNewClientInfo);
#ifdef LOG
	wprintf(L"Session ID : %d, PACKET_SC_CREATE_OHTER_CHARACTER\t New To Other\n",newID);
#endif
	EnqPacketBroadCast((char*)&PSCOC, sizeof(PSCOC), pNewSession->id);
	if (!EnqNewRBForOtherCreate(pNewSession))
		return false;

	return true;
}

void RecvProc(Session* pSession)
{
	int recvRet;
	int peekRet;
	int dqRet;
	if (isDeletedSession(pSession->id))
	{
		return;
	}

	RingBuffer& recvRB = pSession->recvBuffer;
	int recvSize = recvRB.DirectEnqueueSize();
	if (recvSize == 0)
	{
		recvSize = recvRB.GetFreeSize();
	}
	recvRet = recv(pSession->clientSock, recvRB.GetWriteStartPtr(), recvSize, 0);
	if (recvRet == 0)
	{
#ifdef LOG
		wprintf(L"Session ID : %d, TCP CONNECTION END\n", pSession->id);
#endif
		disconnect(pSession->id);
		return;
	}
	else if (recvRet == SOCKET_ERROR)
	{
		int errCode = WSAGetLastError();
		if (errCode != WSAEWOULDBLOCK)
		{
#ifdef LOG
			wprintf(L"Session ID : %d, recv() error code : %d\n", pSession->id, errCode);
#endif
			disconnect(pSession->id);
		}
		return;
	}
#ifdef LOG
	wprintf(L"Session ID : %d, recv() -> recvRB size : %d\n", pSession->id, recvRet);
#endif
	recvRB.MoveRear(recvRet);

	ClientInfo* pCI = g_clientMap.find(pSession->id)->second;
	while (true)
	{
		Header header;
		peekRet = recvRB.Peek(sizeof(header), (char*)&header);
		if (peekRet == 0)
		{
			return;
		}
		if (header.byCode != 0x89)
		{
#ifdef LOG
			wprintf(L"Session ID : %d\tDisconnected by INVALID HeaderCode Packet Received\n", pSession->id);
#endif
			disconnect(pSession->id);
			return;
		}
		char Buffer[BUFFER_SIZE];
		int deqRet = recvRB.Dequeue(Buffer, sizeof(header) + header.bySize);
		if (deqRet == 0)
		{
			return;
		}
		PacketProc(pSession->id, header.byType, Buffer);
	}
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

Session::Session(SOCKET sock, int ID)
	:clientSock{ sock }, id{ ID }
{
}

// 클라이언트의 접속을 끊는 함수들
void disconnect(int id)
{
	g_disconnected_id_set.insert(id);
#ifdef LOG
	wprintf(L"disconnect Session id : %d#\n", id);
#endif 
}

void deleteDisconnectedHelper(int id)
{
	auto SessionIter = g_sessionMap.find(id);
	auto ClientInfoIter = g_clientMap.find(id);
	closesocket(SessionIter->second->clientSock);
	delete SessionIter->second;
	delete ClientInfoIter->second;
	g_sessionMap.erase(id);
	g_clientMap.erase(id);
}
void deleteDisconnected()
{
	for (auto iter = g_disconnected_id_set.begin(); iter != g_disconnected_id_set.end();)
	{
		int id = *iter;
		auto SessionIter = g_sessionMap.find(id);
		auto ClientInfoIter = g_clientMap.find(id);
		closesocket(SessionIter->second->clientSock);
		delete SessionIter->second;
		delete ClientInfoIter->second;
		g_sessionMap.erase(id);
		g_clientMap.erase(id);
		iter = g_disconnected_id_set.erase(iter);
		PACKET_SC_DELETE_CHARACTER PSDC;
		MAKE_PACKET_SC_DELETE_CHATACTER(&PSDC, id);
#ifdef LOG
		wprintf(L"Session id : %d disconnected Broadcast To Other\n", id);
#endif
		EnqPacketBroadCast((char*)&PSDC, sizeof(PSDC), id);
		id_q.push(id);
	}
}

bool EnqNewRBForOtherCreate(IN Session* pNewUser)
{
	for (auto iter = g_clientMap.begin(); iter != g_clientMap.end(); ++iter)
	{
		ClientInfo* pCI = iter->second;
		if (isDeletedSession(pCI->ID) || pCI->ID == pNewUser->id)
			continue;


		PACKET_SC_CREATE_OTHER_CHARARCTER PSCOC;
		MAKE_PACKET_SC_CREATE_OTHER_CHARACTER(&PSCOC, pCI);
		RingBuffer& sendRB = pNewUser->sendBuffer;
		int enqRet = sendRB.Enqueue((char*)&PSCOC, sizeof(PSCOC));
		if (enqRet == 0)
		{
			wprintf(L"Session ID : %d\tsendRingBuffer Full\t", pNewUser->id);
			disconnect(pNewUser->id);
			return false;
		}
#ifdef LOG
		wprintf(L"Session ID : %d  To Session ID : %d\tPACKET_SC_CREATE_OHTER_CHARACTER\t Ohter To New X : %d,Y : %d\n", pCI->ID, pNewUser->id, pCI->x, pCI->y);
		wprintf(L"sendRB Enqueue Size : %d\t sendRB FreeSize : %d\n", enqRet, sendRB.GetFreeSize());
#endif
	}
	return true;

}
void EnqPacketBroadCast(IN char* pPacket, IN const size_t packetSize, IN OPTIONAL const int pTargetIdToExcept)
{
	for (auto iter = g_sessionMap.begin(); iter != g_sessionMap.end(); ++iter)
	{
		Session* pSession = iter->second;
		// 삭제될 예정인 애들 링버퍼에서는 안넣음 , 제외할 세션 링버퍼에는 안넣음
		if (g_disconnected_id_set.find(pSession->id) != g_disconnected_id_set.end() || pSession->id == pTargetIdToExcept)
			continue;

		RingBuffer& sendRB = pSession->sendBuffer;
		int enqRet = sendRB.Enqueue(pPacket, packetSize);
		if (enqRet == 0)
		{
#ifdef LOG
			wprintf(L"Session ID : %d\tsendRingBuffer Full\t", pSession->id);
#endif
			disconnect(pSession->id);
		}
#ifdef LOG
		wprintf(L"Session ID : %d,\t sendRB Enqueue Size : %d\tsendRB FreeSize : %d\n", pSession->id, enqRet, sendRB.GetFreeSize());
#endif LOG
	}

}
// 프레임의 끝부분에서 모든 클라이언트에게 링버퍼 send
void SendProc(Session* pSession)
{
	// 삭제해야하는 id목록에 존재한다면 그냥 넘긴다
	if (g_disconnected_id_set.find(pSession->id) != g_disconnected_id_set.end())
	{
		return;
	}
	RingBuffer& sendRB = pSession->sendBuffer;
	int useSize = sendRB.GetUseSize();
	int directDeqSize = sendRB.DirectDequeueSize();
	while (useSize > 0)
	{
		int sendSize = useSize;
		if (useSize >= directDeqSize && directDeqSize != 0)
		{
			sendSize = directDeqSize;
		}
		else if (directDeqSize == 0)
		{
			sendSize = useSize;
		}
		int sendRet = send(pSession->clientSock, sendRB.GetReadStartPtr(), sendSize, 0);
		if (sendRet == SOCKET_ERROR)
		{
			int errCode = WSAGetLastError();
			if (errCode != WSAEWOULDBLOCK)
			{
				wprintf(L"session ID : %d, send() func error code : %d #\n", pSession->id, errCode);
			}
			else
			{
				__debugbreak();
				wprintf(L"session ID : %d send WSAEWOULDBLOCK #\n", pSession->id);
			}
			disconnect(pSession->id);
			return;
		}
#ifdef LOG
		wprintf(L"session ID : %d\tsend size : %d\tsendRB DirectDequeueSize : %d#\n", pSession->id, sendSize, sendRB.DirectDequeueSize());
#endif
		sendRB.MoveFront(sendSize);
		useSize = sendRB.GetUseSize();
		directDeqSize = sendRB.DirectDequeueSize();
	}
}

inline void HandleMoving(IN ClientInfo* pCI, IN int moveDir)
{
	if (pCI->x - MOVE_UNIT_X <= dfRANGE_MOVE_LEFT)
	{
		return;
	}
	else if (pCI->x + MOVE_UNIT_X >= dfRANGE_MOVE_RIGHT)
	{
		return;
	}

	if (pCI->y - MOVE_UNIT_Y <= dfRANGE_MOVE_TOP)
	{
		return;
	}
	else if (pCI->y + MOVE_UNIT_Y >= dfRANGE_MOVE_BOTTOM)
	{
		return;
	}

	switch (moveDir)
	{
	case dfPACKET_MOVE_DIR_LL:
		pCI->x -= MOVE_UNIT_X;
#ifdef LOG
		wprintf(L"Session ID : %d #\tDIR : LL\tMove Simulate X : %d, Y : %d\n", pCI->ID, pCI->x, pCI->y);
#endif
		break;
	case dfPACKET_MOVE_DIR_LU:
		pCI->x -= MOVE_UNIT_X;
		pCI->y -= MOVE_UNIT_Y;
#ifdef LOG
		wprintf(L"Session ID : %d #\tDIR : LU\tMove Simulate X : %d, Y : %d\n", pCI->ID, pCI->x, pCI->y);
#endif
		break;
	case dfPACKET_MOVE_DIR_UU:
		pCI->y -= MOVE_UNIT_Y;
#ifdef LOG
		wprintf(L"Session ID : %d #\tDIR : UU\tMove Simulate X : %d, Y : %d\n", pCI->ID, pCI->x, pCI->y);
#endif
		break;
	case dfPACKET_MOVE_DIR_RU:
		pCI->x += MOVE_UNIT_X;
		pCI->y -= MOVE_UNIT_Y;
#ifdef LOG
		wprintf(L"Session ID : %d #\tDIR : RU\tMove Simulate X : %d, Y : %d\n", pCI->ID, pCI->x, pCI->y);
#endif
		break;
	case dfPACKET_MOVE_DIR_RR:
		pCI->x += MOVE_UNIT_X;
#ifdef LOG
		wprintf(L"Session ID : %d #\tDIR : RR\tMove Simulate X : %d, Y : %d\n", pCI->ID, pCI->x, pCI->y);
#endif
		break;
	case dfPACKET_MOVE_DIR_RD:
		pCI->x += MOVE_UNIT_X;
		pCI->y += MOVE_UNIT_Y;
#ifdef LOG
		wprintf(L"Session ID : %d #\tDIR : RD\tMove Simulate X : %d, Y : %d\n", pCI->ID, pCI->x, pCI->y);
#endif
		break;
	case dfPACKET_MOVE_DIR_DD:
		pCI->y += MOVE_UNIT_Y;
#ifdef LOG
		wprintf(L"Session ID : %d #\tDIR : DD\tMove Simulate X : %d, Y : %d\n", pCI->ID, pCI->x, pCI->y);
#endif
		break;
	case dfPACKET_MOVE_DIR_LD:
		pCI->x -= MOVE_UNIT_X;
		pCI->y += MOVE_UNIT_Y;
#ifdef LOG
		wprintf(L"Session ID : %d #\tDIR : LD\tMove Simulate X : %d, Y : %d\n", pCI->ID, pCI->x, pCI->y);
#endif
		break;
	default:
		__debugbreak();
		return;
	}
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
}
