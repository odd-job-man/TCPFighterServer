#pragma once
#include <WinSock2.h>
#include "RingBuffer.h"
#include "Network.h"
constexpr char INIT_DIR = dfPACKET_MOVE_DIR_LL;
constexpr int INIT_POS_X = 300;
constexpr int INIT_POS_Y = 300;
constexpr int INIT_HP = 100;
constexpr int dfRANGE_MOVE_TOP = 50;
constexpr int dfRANGE_MOVE_LEFT = 10;
constexpr int dfRANGE_MOVE_RIGHT = 630;
constexpr int dfRANGE_MOVE_BOTTOM = 470;

//---------------------------------------------------------------
// 공격범위.
//---------------------------------------------------------------
constexpr int dfATTACK1_RANGE_X = 80;
constexpr int dfATTACK2_RANGE_X = 90;
constexpr int dfATTACK3_RANGE_X = 100;
constexpr int dfATTACK1_RANGE_Y = 10;
constexpr int dfATTACK2_RANGE_Y = 10;
constexpr int dfATTACK3_RANGE_Y = 20;

struct ClientInfo
{
	int ID;
	int hp;
	int x;
	int y;
	char viewDir;
	int action;
};


#define IN
#define OPTIONAL
#define OUT
void InitClientInfo(OUT ClientInfo* pCI, IN int id);
