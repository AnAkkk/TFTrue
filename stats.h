/*
 * Copyright (C) 2015 AnAkkk <anakin.cs@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

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
