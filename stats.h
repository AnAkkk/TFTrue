#pragma once

#include "ModuleScanner.h"

#include "SDK.h"

#include "valve_minmax_off.h"
#include <map>

class CStats: public IGameEventListener2
{
public:
	void OnUnload();
	void OnDisconnect(edict_t *pEntity);
	void OnJoinTeam(edict_t *pEntity);
	void OnServerActivate();

	bool Init(const CModuleScanner& ServerModule);

	void SendStatsToPlayer( CBasePlayer *pPlayer, int *uiStats );

	virtual void FireGameEvent(IGameEvent* pEvent );

private:
	void Reset();

	// Save and restore stats for tftrue_restorestats
	std::map<AccountID_t, int*> vecScore;
	unsigned char ucTFSTAT_MAX = 45;
	unsigned int uiStatsAccumulatedOffset = 360;

	void *FindPlayerStats = nullptr;
	void *AccumulateAndResetPerLifeStats = nullptr;

	/*CTFGameStats*/void *pTFGameStats = nullptr;

	typedef void * (__thiscall *FindPlayerStatsFn)( void *thisPtr, CBasePlayer *pPlayer );
	typedef void (__thiscall *AccumulateAndResetPerLifeStatsFn)( void *thisPtr, CBasePlayer *pPlayer);
};

extern CStats g_Stats;

extern ConVar tftrue_restorestats;
