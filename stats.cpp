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

#include "stats.h"
#include "MRecipient.h"
#include "utils.h"

#include "player_resource.h"

CStats g_Stats;

ConVar tftrue_restorestats("tftrue_restorestats", "1", FCVAR_NOTIFY,
    "Restore stats of players if they accidentally disconnect and reconnect to the server.",
    true, 0, true, 1);

bool CStats::Init(const CModuleScanner& ServerModule)
{
	char* os;

#ifdef _LINUX

	os = (char*)"Linux";

	FindPlayerStats                         = ServerModule.FindSymbol(
	"_ZN12CTFGameStats15FindPlayerStatsEP11CBasePlayer");
	AccumulateAndResetPerLifeStats          = ServerModule.FindSymbol(
	"_ZN12CTFGameStats30AccumulateAndResetPerLifeStatsEP9CTFPlayer");

	if (AccumulateAndResetPerLifeStats)
	{
	    ucTFSTAT_MAX                        = *(unsigned char*)((unsigned char*)
	    AccumulateAndResetPerLifeStats + 0x6C);
	}

	pTFGameStats = (void*)ServerModule.FindSymbol("CTF_GameStats");

#else

	os = (char*)"Windows";

	FindPlayerStats                         = ServerModule.FindSignature((unsigned char *)
	"\x55\x8B\xEC\x8B\x45\x08\x85\xC0\x75\x04\x5D", "xxxxxxxxxxx");
	AccumulateAndResetPerLifeStats          = ServerModule.FindSignature((unsigned char*)
	"\x55\x8B\xEC\x51\x53\x56\x8B\x75\x08\x57\x8B\xF9\x8B\xCE\x89\x7D\xFC\x8B\x06", "xxxxxxxxxxxxxxxxxxx");

	if(AccumulateAndResetPerLifeStats)
	{
	    ucTFSTAT_MAX                        = *(unsigned char*)((unsigned char*)
	    AccumulateAndResetPerLifeStats + 0xAC);
	}

	// somewhere in CTFGameRules::SendArenaWinPanelInfo
	void *pSendArenaWinPanelInfo = ServerModule.FindSignature((unsigned char *)
	"\xB9\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x8D\x4D\xD4\x89\x45\xF4", "x????x????xxxxxx");

	if (pSendArenaWinPanelInfo)
	{
	    pTFGameStats = *(void **)((unsigned char*)pSendArenaWinPanelInfo + 1);
	}
	else
	{
	    Warn("Couldn't get sig for pSendArenaWinPanelInfo! OS: %s\n", os);
	}
#endif

	if(!FindPlayerStats)
	{
		Warning("Couldn't get sig for FindPlayerStats! OS: %s\n", os);
	}
	if (!AccumulateAndResetPerLifeStats)
	{
		Warning("Couldn't get sig for AccumulateAndResetPerLifeStats! OS: %s\n", os);
	}
	if(!pTFGameStats)
	{
		Warning("Couldn't get sig for pTFGameStats! OS: %s\n", os);
	}

	gameeventmanager->AddListener(this, "teamplay_restart_round", true);

	return (FindPlayerStats && AccumulateAndResetPerLifeStats && pTFGameStats);
}

void CStats::Reset()
{
	for(const auto &i : vecScore)
		delete[] i.second;

	vecScore.clear();
}

void CStats::OnUnload()
{
	Reset();
	gameeventmanager->RemoveListener(this);
}

void CStats::FireGameEvent(IGameEvent *pEvent)
{
	if(!strcmp(pEvent->GetName(), "teamplay_restart_round"))
		Reset();
}

void CStats::OnServerActivate()
{
	// Remove everything on map change
	for(auto const &i : vecScore)
		delete[] i.second;

	vecScore.clear();
}

void CStats::OnJoinTeam(edict_t *pEntity)
{
	if(!tftrue_restorestats.GetBool())
		return;

	const CSteamID *steamid = engine->GetClientSteamID(pEntity);
	if(steamid)
	{
		auto it = vecScore.find(steamid->GetAccountID());

		if(it != vecScore.end())
		{
			CBasePlayer *pPlayer = (CBasePlayer*)CBaseEntity::Instance(pEntity);
			int *uiAccumulatedStatsSave = it->second;

			void *pStats = reinterpret_cast<FindPlayerStatsFn>(FindPlayerStats)(pTFGameStats, (CBasePlayer*)CBaseEntity::Instance(pEntity));
			memcpy((char*)pStats+StatsAccumulatedOffset, uiAccumulatedStatsSave, ucTFSTAT_MAX*4);
			SendStatsToPlayer(pPlayer, uiAccumulatedStatsSave);

			// Restore frags and deaths
			// for scoreboard

			/*
			- m_iFrags      (Offset 3404) (Save)(4 Bytes)
			- m_iDeaths     (Offset 3408) (Save)(4 Bytes)

			- m_hViewModel  (Offset 3436) (Save)(8 Bytes)
			*/

			// m_iFrags
			*(int*)(g_EntityProps.GetSendProp<char>(pPlayer, "m_hViewModel")-32) = uiAccumulatedStatsSave[3];
			// m_iDeaths
			*(int*)(g_EntityProps.GetSendProp<char>(pPlayer, "m_hViewModel")-28) = uiAccumulatedStatsSave[4];

			// for A2S_PLAYERS
			CPlayerState *pl = g_pGameClients->GetPlayerState(pEntity);
			if(pl)
			{
				pl->frags = uiAccumulatedStatsSave[3];
				pl->deaths = uiAccumulatedStatsSave[4];
			}

			vecScore.erase(it);
			delete[] uiAccumulatedStatsSave;
		}
	}
}

void CStats::OnDisconnect(edict_t *pEntity)
{
	int iEntIndex = IndexOfEdict(pEntity);

	if(tftrue_restorestats.GetBool())
	{
		CPlayerResource *pPlayerResource = (CPlayerResource*)g_pServerTools->FindEntityByClassname(nullptr, "tf_player_manager");

		if(pPlayerResource)
		{
			int iScore = *(int*)(g_EntityProps.GetSendProp<char>(pPlayerResource, "m_iTotalScore")+iEntIndex*4);

			// Check if we don't already have stats for the player
			// It could happen that some stats were saved, he reconnected, didn't join a team and disconnected
			// so we shouldn't save the stats again or we'll be leaking memory!
			bool bAlreadyHasStats = false;
			const CSteamID *steamid = engine->GetClientSteamID(pEntity);
			if(steamid)
			{
				auto it = vecScore.find(steamid->GetAccountID());
				if(it != vecScore.end())
					bAlreadyHasStats = true;
			}

			if(iScore > 0 && !bAlreadyHasStats)
			{
				CBasePlayer *pPlayer = (CBasePlayer*)CBaseEntity::Instance(pEntity);
				if(pPlayer)
				{
					reinterpret_cast<AccumulateAndResetPerLifeStatsFn>(AccumulateAndResetPerLifeStats)(pTFGameStats, pPlayer);
					void *pStats = reinterpret_cast<FindPlayerStatsFn>(FindPlayerStats)(pTFGameStats, pPlayer);
					if(pStats)
					{
						int* uiAccumulatedStatsSave = new int[ucTFSTAT_MAX];
						memcpy(uiAccumulatedStatsSave, (char*)pStats+StatsAccumulatedOffset, ucTFSTAT_MAX*4);

						if(steamid)
							vecScore[steamid->GetAccountID()] = uiAccumulatedStatsSave;
					}
				}
			}
		}
	}
}

void CStats::SendStatsToPlayer( CBasePlayer *pPlayer, int *uiStats )
{
	int iSendBits = 0;

	for ( int i = 1; i < ucTFSTAT_MAX; i++ )
	{
		if(uiStats[i] > 0)
			iSendBits |= ( 1 << ( i - 1 ) );
	}

	int iStat = 1;
	MRecipientFilter filter;
	filter.AddRecipient(pPlayer->entindex());
	bf_write *pBuffer = engine->UserMessageBegin( &filter, GetMessageType("PlayerStatsUpdate") );
	// write the class
	pBuffer->WriteByte( *g_EntityProps.GetSendProp<int>(pPlayer, "m_PlayerClass.m_iClass") );
	// alive or dead
	pBuffer->WriteByte( true );
	// write the bit mask of which stats follow in the message
	pBuffer->WriteLong( iSendBits );
	// write all the stats specified in the bit mask
	while ( iSendBits > 0 )
	{
		if ( iSendBits & 1 )
		{
			pBuffer->WriteLong( uiStats[iStat] );
		}
		iSendBits >>= 1;
		iStat++;
	}
	engine->MessageEnd();
}
