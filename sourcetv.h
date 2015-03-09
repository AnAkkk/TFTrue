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
#include "FunctionRoute.h"

class CSourceTV
{
public:
	CSourceTV();

	bool Init();
	void OnUnload();
	void OnGameOver();
	void OnTournamentStateUpdate(const char *szBlueTeamName, const char *szRedTeamName);
	void OnServerActivate();

	void StopTVRecord();
	void UpdateMapChangeDelay();
	bool IsDelayingMapChange() { return m_bDelayMapChange; }

	static void AutoRecord_Callback( IConVar *var, const char *pOldValue, float flOldValue );
	static void Enable_Callback( IConVar *var, const char *pOldValue, float flOldValue );
	static void Prefix_Callback( IConVar *var, const char *pOldValue, float flOldValue );
	static void __fastcall ChangeLevel_Callback(ConCommand *pCmd, const CCommand &args);

private:
	char m_szTVRecord[255];
	FnChangeCallback_t m_Enable_OldCallback = nullptr;

	bool m_bDelayMapChange = false;
	float m_flTvDelay = 0.0f;
	float m_flNextMessage = 0.0f;

	CFunctionRoute m_DispatchChangeLevelRoute;
};

extern CSourceTV g_SourceTV;

extern ConVar tftrue_tv_delaymapchange;
extern ConVar tftrue_tv_autorecord;
extern ConVar tftrue_tv_recordpath;
extern ConVar tftrue_tv_prefix;
