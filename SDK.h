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

#define GAME_DLL 1

// Fix INVALID_HANDLE_VALUE redefinition warning
#ifndef _LINUX
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#endif

#include "filesystem.h"
#include "iserver.h"
#include "iclient.h"
#include "inetchannel.h"
#include "replay/ienginereplay.h"

#include "cbase.h"
#include "gamemovement.h"

#include "tier0/icommandline.h"
#include "toolframework/itoolentity.h"

#include "steam/steam_api.h"
#include "steam/steam_gameserver.h"

extern IVEngineServer	*engine;
extern IPlayerInfoManager *playerinfomanager;
extern ICvar *g_pCVar;
extern IServerGameDLL *gamedll;
extern IServerGameEnts *gameents;
extern IFileSystem *filesystem;
extern IServerPluginHelpers* helpers;
extern IServer* g_pServer;
extern CGlobalVars *gpGlobals;
extern CGlobalEntityList *pEntList;
extern CGameMovement* g_pGameMovement;
extern IServerGameClients* g_pGameClients;
extern IEngineTrace* g_pEngineTrace;
extern IServerTools* g_pServerTools;

enum tfclass_t
{
	TFCLASS_SCOUT = 1,
	TFCLASS_SNIPER = 2,
	TFCLASS_SOLDIER = 3,
	TFCLASS_DEMOMAN = 4,
	TFCLASS_MEDIC = 5,
	TFCLASS_HEAVY = 6,
	TFCLASS_PYRO = 7,
	TFCLASS_SPY = 8,
	TFCLASS_ENGINEER = 9,
};

enum {
	TF_CUSTOM_HEADSHOT = 1,
	TF_CUSTOM_BACKSTAB,
	TF_CUSTOM_BURNING,
	TF_CUSTOM_WRENCH_FIX,
	TF_CUSTOM_MINIGUN,
	TF_CUSTOM_SUICIDE,
	TF_CUSTOM_TAUNT_HADOUKEN,
	TF_CUSTOM_BURNING_FLARE,
	TF_CUSTOM_TAUNT_HIGH_NOON,
	TF_CUSTOM_TAUNT_GRAND_SLAM,
	TF_CUSTOM_PENETRATE_MY_TEAM,
	TF_CUSTOM_PENETRATE_ALL_PLAYERS,
	TF_CUSTOM_TAUNT_FENCING,
	TF_CUSTOM_PENETRATE_HEADSHOT,
	TF_CUSTOM_TAUNT_ARROW_STAB,
	TF_CUSTOM_TELEFRAG,
	TF_CUSTOM_BURNING_ARROW,
	TF_CUSTOM_FLYINGBURN,
	TF_CUSTOM_PUMPKIN_BOMB,
	TF_CUSTOM_DECAPITATION,
	TF_CUSTOM_TAUNT_GRENADE,
	TF_CUSTOM_BASEBALL,
	TF_CUSTOM_CHARGE_IMPACT,
	TF_CUSTOM_TAUNT_BARBARIAN_SWING,
	TF_CUSTOM_AIR_STICKY_BURST,
	TF_CUSTOM_DEFENSIVE_STICKY,
	TF_CUSTOM_PICKAXE,
	TF_CUSTOM_ROCKET_DIRECTHIT,
	TF_CUSTOM_TAUNT_UBERSLICE,
	TF_CUSTOM_PLAYER_SENTRY,
	TF_CUSTOM_STANDARD_STICKY,
	TF_CUSTOM_SHOTGUN_REVENGE_CRIT,
	TF_CUSTOM_TAUNT_ENGINEER_SMASH,
	TF_CUSTOM_BLEEDING,
	TF_CUSTOM_GOLD_WRENCH,
	TF_CUSTOM_CARRIED_BUILDING,
	TF_CUSTOM_COMBO_PUNCH,
	TF_CUSTOM_TAUNT_ENGINEER_ARM,
	TF_CUSTOM_FISH_KILL,
	TF_CUSTOM_TRIGGER_HURT,
	TF_CUSTOM_DECAPITATION_BOSS,
	TF_CUSTOM_STICKBOMB_EXPLOSION,
	TF_CUSTOM_AEGIS_ROUND,
	TF_CUSTOM_FLARE_EXPLOSION,
	TF_CUSTOM_BOOTS_STOMP,
	TF_CUSTOM_PLASMA,
	TF_CUSTOM_PLASMA_CHARGED,
	TF_CUSTOM_PLASMA_GIB,
	TF_CUSTOM_PRACTICE_STICKY,
	TF_CUSTOM_EYEBALL_ROCKET,
	TF_CUSTOM_HEADSHOT_DECAPITATION,
	TF_CUSTOM_TAUNT_ARMAGEDDON,
	TF_CUSTOM_FLARE_PELLET,
	TF_CUSTOM_CLEAVER,
	TF_CUSTOM_CLEAVER_CRIT,
	TF_CUSTOM_SAPPER_RECORDER_DEATH,
	TF_CUSTOM_MERASMUS_PLAYER_BOMB,
	TF_CUSTOM_MERASMUS_GRENADE,
	TF_CUSTOM_MERASMUS_ZAP,
	TF_CUSTOM_MERASMUS_DECAPITATION,
	TF_CUSTOM_CANNONBALL_PUSH,
};


//m_nPlayerCond
#define TF2_PLAYERCOND_SLOWED (1<<0) // Zoomed / Arrow drawn / minigun spinning
#define TF2_PLAYERCOND_ZOOMED (1<<1) // FOV change (sniper)
#define TF2_PLAYERCOND_DISGUISING (1<<2)
#define TF2_PLAYERCOND_DISGUISED (1<<3)
#define TF2_PLAYERCOND_SPYCLOAK (1<<4) // Spy is cloaking
#define TF2_PLAYERCOND_UBERCHARGED (1<<5)
#define TF2_PLAYERCOND_TELEGLOW (1<<6)
#define TF2_PLAYERCOND_TAUNT (1<<7)
#define TF2_PLAYERCOND_UBERFADING (1<<8)
//(1<<9) ??
#define TF2_PLAYERCOND_TELEPORTING (1<<10) // Player is teleporting
#define TF2_PLAYERCOND_CRITS (1<<11) // Player is crit boosted
#define TF2_PLAYERCOND_TMPDAMAGEBONUS (1<<12) // ??
#define TF2_PLAYERCOND_FEIGNDEATH (1<<13) // Feign death with dead ringer
#define TF2_PLAYERCOND_BONKD (1<<14) // Atomic bonk
#define TF2_PLAYERCOND_DAZED (1<<15) // When a player gets dazed (atomic bomb / airblasted / baseball bat)
#define TF2_PLAYERCOND_SOLDIERBUFFBANNER (1<<16)
#define TF2_PLAYERCOND_CHARGE (1<<17) // Demoman charge
#define TF2_PLAYERCOND_HEADGAINED (1<<18) // Demoman decap - first only
#define TF2_PLAYERCOND_HEALING (1<<19) // Player is being healed // Energy drink buff
//(1<<20) ??
#define TF2_PLAYERCOND_OVERHEAL (1<<21) // Player is overhealed
#define TF2_PLAYERCOND_BURNING (1<<22) // Player is on fire
#define TF2_PLAYERCOND_UBERCHARGE (1<<23) // Player is ubercharged
#define TF2_PLAYERCOND_JARATE (1<<24) // Player is jarated
#define TF2_PLAYERCOND_BLEEDING (1<<25)
#define TF2_PLAYERCOND_SOLDIERDEFENSEBUFF (1<<26)
#define TF2_PLAYERCOND_SOLDIERREGENBUFF (1<<27)
//(1<<28) ??
#define TF2_PLAYERCOND_SOLDIERUNKNOWN (1<<29)
//(1<<30) ??
#define TF2_PLAYERCOND_SOLDIERUNKNOWN1 (1<<31)
#define TF2_PLAYERCOND_SPEED (1<<32) // ?


// Might not be a good idea, does __thiscall already exists within clang?
#ifndef _LINUX
#define EDX int edx,
#define EDX2 , int edx
#else
#define __thiscall
#define __fastcall
#define __cdecl
#define EDX
#define EDX2
#endif

class CPlugin
{
public:
	char m_szName[128];
	bool m_bDisable;

	IServerPluginCallbacks *m_pPlugin;
	int m_iPluginInterfaceVersion;
	CSysModule	*m_pPluginModule;
};
