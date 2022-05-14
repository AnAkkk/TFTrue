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

#include "SDK.h"

#include "valve_minmax_off.h"
#include "FunctionRoute.h"
#include "ModuleScanner.h"

#include <bitset>

class MyCGameMovement: public CGameMovement
{
public:
	virtual bool	CheckJumpButton( void );
};

class CBunnyHop: public IGameEventListener2
{
public:
	bool Init(const CModuleScanner& ServerModule);
	void OnJoinClass(edict_t *pEntity);
	void OnPlayerDisconnect(edict_t *pEntity);
	void OnUnload();
	void FireGameEvent(IGameEvent *pEvent);

	void SetSpeedMeter(int iEntIndex, bool bEnabled) { m_bSpeedMeter[iEntIndex] = bEnabled; }

	static void Callback( IConVar *var, const char *pOldValue, float flOldValue );
	static bool __fastcall CheckJumpButton(CGameMovement *pMovement EDX2);

	static void PreventBunnyJumping();
private:
	void *PreventBunnyJumpingAddr = nullptr;

	std::bitset<MAX_PLAYERS+1> m_bSpeedMeter;

	CFunctionRoute PreventBunnyJumpingRoute;
	CFunctionRoute CheckJumpButtonRoute;

	typedef bool (__thiscall *CheckJumpButton_t)( void *thisPtr );
};

extern CBunnyHop g_BunnyHop;
extern ConVar tftrue_bunnyhop;
