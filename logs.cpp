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

#include "logs.h"
#include "utils.h"
#include "items.h"
#include "tournament.h"

#include <thread>
#include <json/json.h>
#include <sys/stat.h>

#ifdef WIN32
#include <WS2tcpip.h>
#endif

CLogs g_Logs;
CLog *g_Log = nullptr;

ConVar tftrue_logs_apikey("tftrue_logs_apikey", "", FCVAR_NONE|FCVAR_PROTECTED,
	"Set the API key to upload logs to logs.tf",
	&CLogs::ApiKey_Callback);

ConVar tftrue_logs_name_prefix("tftrue_logs_prefix", "", FCVAR_NONE,
	"Prefix to add in the log name when uploading to logs.tf");

ConVar tftrue_logs_includebuffs("tftrue_logs_includebuffs", "1", FCVAR_NOTIFY,
	"Includes buffs within the player_healed event in the logs.",
	true, 0, true, 1);

ConVar tftrue_logs_accuracy("tftrue_logs_accuracy", "0", FCVAR_NOTIFY,
	"Log shots and hits for accuracy stats.",
	true, 0, true, 1);

ConVar tftrue_logs_roundend("tftrue_logs_roundend", "1", FCVAR_NOTIFY,
	"Upload logs at every round end.",
	true, 0, true, 1);

CLogs::CLogs()
{
	memset(m_uiLastHealOnHit, 0, sizeof(m_uiLastHealOnHit));
	memset(m_uiLastDirectHitVictim, 0, sizeof(m_uiLastDirectHitVictim));
	memset(m_szCurrentLogFile, 0, sizeof(m_szCurrentLogFile));
	memset(m_szCurrentLogFileBackup, 0, sizeof(m_szCurrentLogFileBackup));
	memset(m_szLastUploadedLog, 0, sizeof(m_szLastUploadedLog));
}

bool CLogs::Init(const CModuleScanner& EngineModule, const CModuleScanner& ServerModule)
{
	char* os;

#ifdef _LINUX

	os = (char*)"Linux";

	void *Event_PlayerHealedOther                  = ServerModule.FindSymbol(
	"_ZN12CTFGameStats23Event_PlayerHealedOtherEP9CTFPlayerf");
	void *Event_PlayerFiredWeapon                  = ServerModule.FindSymbol(
	"_ZN12CTFGameStats23Event_PlayerFiredWeaponEP9CTFPlayerb");
	void *Event_PlayerDamage                       = ServerModule.FindSymbol(
	"_ZN12CTFGameStats18Event_PlayerDamageEP11CBasePlayerRK15CTakeDamageInfoi");
	GetKillingWeaponName                           = ServerModule.FindSymbol(
	"_ZN12CTFGameRules20GetKillingWeaponNameERK15CTakeDamageInfoP9CTFPlayerPi");
	void *OnTakeDamage                             = ServerModule.FindSymbol(
	"_ZN9CTFPlayer12OnTakeDamageERK15CTakeDamageInfo");
	g_Log                                          = (CLog*)EngineModule.FindSymbol("g_Log");

#else

	os = (char*)"Windows";


	void *Event_PlayerHealedOther                  = ServerModule.FindSignature((unsigned char*)
//   .text:105CD910 128 call    sub_10446050    ; Call Procedure
//?? \x55\x8B\xEC\x81\xEC\x10\x01\x00\x00\x53\x56\x57\x8B\x7D\x0C
	"\x55\x8B\xEC\xF3\x0F\x2C\x45\x00\x57", "xxxxxxx?x");
	void *Event_PlayerFiredWeapon                  = ServerModule.FindSignature((unsigned char*)
	"\x55\x8B\xEC\xA1\x00\x00\x00\x00\x56\x8B\x75\x08\x57\x83\xB8", "xxxx????xxxxxxx");
	void *Event_PlayerDamage                       = ServerModule.FindSignature((unsigned char*)
	"\x55\x8B\xEC\x83\xEC\x00\x53\x8B\x5D\x10\x89\x4D\xFC\x85", "xxxxx?xxxxxxxx");
	GetKillingWeaponName                           = ServerModule.FindSignature((unsigned char*)
	"\x55\x8B\xEC\x83\xEC\x18\x53\x8B\xD9\x8B\x4D\x08\x56", "xxxxxxxxxxxxx");
	void *OnTakeDamage                             = ServerModule.FindSignature((unsigned char*)
	"\x55\x8B\xEC\x81\xEC\x00\x00\x00\x00\x56\x8B\x75\x08\x57\x8B\xF9\x8D\x8D\x00\x00\x00\x00\x56", "xxxxx????xxxxxxxxx????x");
	void *pLog                                     = EngineModule.FindSignature((unsigned char*)
	"\xB9\x00\x00\x00\x00\xE8\x00\x00\x00\x00\xC6\x05\x00\x00\x00\x00\x00\x5E", "x????x????xx?????x");
	if(pLog)
	g_Log = *(CLog**)((char*)pLog+1);

#endif

	if(!Event_PlayerHealedOther)
	{
		Warning("Can't get sig for Event_PlayerHealedOther! OS: %s\n", os);
	}
	else
	{
		Event_PlayerHealedOtherRoute.RouteFunction(Event_PlayerHealedOther, (void*)CLogs::Event_PlayerHealedOther);
	}
	if(!Event_PlayerFiredWeapon)
	{
		Warning("Can't get sig for Event_PlayerFiredWeapon! OS: %s\n", os);
	}
	else
	{
		Event_PlayerFiredWeaponRoute.RouteFunction(Event_PlayerFiredWeapon, (void*)CLogs::Event_PlayerFiredWeapon);
	}
	if(!GetKillingWeaponName)
	{
		Warning("Can't get sig for GetKillingWeaponName! OS: %s\n", os);
	}
	if(!OnTakeDamage)
	{
		Warning("Can't get sig for OnTakeDamage! OS: %s\n", os);
	}
	// Can't give an offset in CFunctionRoute, try to hook it a non virtual way
	else
	{
		OnTakeDamageRoute.RouteFunction(OnTakeDamage, (void*)CLogs::OnTakeDamage);
	}
	if(!Event_PlayerDamage)
	{
		Warning("Can't get sig for Event_PlayerDamage! OS: %s\n", os);
	}
	else
	{
		Event_PlayerDamageRoute.RouteFunction(Event_PlayerDamage, (void*)CLogs::Event_PlayerDamage);
	}

	LogPrintRoute.RouteVirtualFunction(engine, &IVEngineServer::LogPrint, &CLogs::LogPrint);

	OpenExRoute.RouteVirtualFunction(filesystem, &IFileSystem::OpenEx, &CLogs::OpenEx);

	if(!g_Log)
	{
		Warning("Can't get sig for g_Log! OS: %s\n", os);
	}

	gameeventmanager->AddListener(this, "item_pickup", true);
	gameeventmanager->AddListener(this, "player_healed", true);
	gameeventmanager->AddListener(this, "player_spawn", true);
	gameeventmanager->AddListener(this, "player_chargedeployed", true);
	gameeventmanager->AddListener(this, "player_healonhit", true);
	gameeventmanager->AddListener(this, "teamplay_round_start", true);
	gameeventmanager->AddListener(this, "projectile_direct_hit", true);
	gameeventmanager->AddListener(this, "crossbow_heal", true);
	return Event_PlayerHealedOther && Event_PlayerFiredWeapon &&
			Event_PlayerDamage && GetKillingWeaponName && OnTakeDamage && g_Log;
}

void CLogs::OnUnload()
{
	for(int i = 0; i < 34; i++)
		mapHealing[i].clear();

	gameeventmanager->RemoveListener(this);
}

void CLogs::OnGameFrame()
{
	LogAllHealing();
	LogUberchargeStatus();
}

void CLogs::OnDisconnect(edict_t *pEntity)
{
	int iEntIndex = IndexOfEdict(pEntity);

	for(int i = 0; i < 34; i++)
	{
		if(mapHealing[i].size() > 0)
		{
			for(auto it = mapHealing[i].begin(); it != mapHealing[i].end();)
			{
				// If it was a patient or a healer
				if(it->first == iEntIndex || i == iEntIndex-1)
				{
					LogHealing(i+1, it->first, it->second);
					it = mapHealing[i].erase(it);
				}
				else
					it++;
			}
		}
	}
}

void CLogs::OnGameOver()
{
	static ConVarRef mp_tournament("mp_tournament");

	if(strcmp(tftrue_logs_apikey.GetString(), "") && g_Log->m_bActive && mp_tournament.GetBool())
	{
		// Before uploading, save the current log as we need to close it with "log on"
		V_strncpy(m_szCurrentLogFileBackup, m_szCurrentLogFile, sizeof(m_szCurrentLogFileBackup));

		// Create a new log
		engine->InsertServerCommand("log on\n");
		engine->ServerExecute();

		// If we have already tried to upload in the last 20 seconds, don't try again
		// This means tftrue_logs_roundend is probably enabled and we already uploaded due to a
		// round end
		if(gpGlobals->curtime - m_flLastLogUploadTriedTime > 20.0f)
		{
			Upload(false);
			m_flLastLogUploadTriedTime = gpGlobals->curtime;
		}
		else
			m_iLastLogID = 0;
	}
}

void CLogs::OnRoundWin()
{
	static ConVarRef mp_tournament("mp_tournament");

	if(strcmp(tftrue_logs_apikey.GetString(), "") && g_Log->m_bActive && mp_tournament.GetBool())
	{
		if(tftrue_logs_roundend.GetBool())
		{
			// Before uploading, save the current log because we always read it from szCurrentLogFileBackup
			V_strncpy(m_szCurrentLogFileBackup, m_szCurrentLogFile, sizeof(m_szCurrentLogFileBackup));

			// Flush the current log
			if(g_Log->m_hLogFile != FILESYSTEM_INVALID_HANDLE)
				filesystem->Flush(g_Log->m_hLogFile);

			Upload(true);

			m_flLastLogUploadTriedTime = gpGlobals->curtime;
		}
	}
}

void CLogs::LogUberchargeStatus()
{
	static ConVarRef mp_tournament("mp_tournament");
	static ConVarRef tf_gamemode_mvm("tf_gamemode_mvm");

	for(int i = 1; i <= g_pServer->GetClientCount(); i++)
	{
		CBasePlayer *pPlayer = (CBasePlayer*)CBaseEntity::Instance(i);
		if(!pPlayer)
			continue;

		if(*g_EntityProps.GetSendProp<int>(pPlayer, "m_PlayerClass.m_iClass") != TFCLASS_MEDIC)
			continue;

		if(*g_EntityProps.GetSendProp<char>(pPlayer, "m_lifeState") != LIFE_ALIVE)
			continue;

		int iTeamNum = *g_EntityProps.GetSendProp<int>(pPlayer, "m_iTeamNum");

		float flChargeLevel = GetChargeLevel(pPlayer);

		if(flChargeLevel == 100.0f &&
				((iTeamNum == 2 && m_flRedMedicHasUberChargeTime == 0.0f) || (iTeamNum == 3 && m_flBlueMedicHasUberChargeTime == 0.0f)))
		{
			(iTeamNum == 2 ? m_flRedMedicHasUberChargeTime : m_flBlueMedicHasUberChargeTime) = gpGlobals->curtime;

			if(mp_tournament.GetBool() && !tf_gamemode_mvm.GetBool())
			{
				IPlayerInfo *pInfo = playerinfomanager->GetPlayerInfo(gameents->BaseEntityToEdict(pPlayer));
				char szTeam[5] = "";

				if(iTeamNum == 2)
				{
					V_strncpy(szTeam, "Red", sizeof(szTeam));
					if(m_flBlueMedicHasUberChargeTime > 0.0f && gpGlobals->curtime - m_flBlueMedicHasUberChargeTime >= 10.0f)
						m_flBlueMedicLostUberAdvantage = gpGlobals->curtime - m_flBlueMedicHasUberChargeTime;
				}
				else if(iTeamNum == 3)
				{
					V_strncpy(szTeam, "Blue", sizeof(szTeam));
					if(m_flRedMedicHasUberChargeTime > 0.0f && gpGlobals->curtime - m_flRedMedicHasUberChargeTime >= 10.0f)
						m_flRedMedicLostUberAdvantage = gpGlobals->curtime - m_flRedMedicHasUberChargeTime;
				}

				char msg[128];
				V_snprintf(msg, sizeof(msg),
						   "\"%s<%d><%s><%s>\" triggered \"chargeready\"\n",
						   pInfo->GetName(), pInfo->GetUserID(),
						   pInfo->GetNetworkIDString(), szTeam);
				engine->LogPrint(msg);
			}
		} else if (flChargeLevel == 0.0f &&
				   ((iTeamNum == 2 && m_flRedMedicHasUberChargeTime != 0.0f) || (iTeamNum == 3 && m_flBlueMedicHasUberChargeTime != 0.0f)))
		{
			((iTeamNum == 2) ? (m_flRedMedicHasUberChargeTime) : (m_flBlueMedicHasUberChargeTime)) = 0.0f;

			if(mp_tournament.GetBool() && !tf_gamemode_mvm.GetBool())
			{
				IPlayerInfo *pInfo = playerinfomanager->GetPlayerInfo(gameents->BaseEntityToEdict(pPlayer));
				char szTeam[5] = "";

				float flChargeDuration = 0.0f;

				if(iTeamNum == 2)
				{
					V_strncpy(szTeam, "Red", sizeof(szTeam));
					flChargeDuration = gpGlobals->curtime - m_flRedUberChargeStartTime;
				}
				else if(iTeamNum == 3)
				{
					V_strncpy(szTeam, "Blue", sizeof(szTeam));
					flChargeDuration = gpGlobals->curtime - m_flBlueUberChargeStartTime;
				}

				char msg[128];
				V_snprintf(msg, sizeof(msg),
						   "\"%s<%d><%s><%s>\" triggered \"empty_uber\"\n",
						   pInfo->GetName(), pInfo->GetUserID(),
						   pInfo->GetNetworkIDString(), szTeam);
				engine->LogPrint(msg);

				V_snprintf(msg, sizeof(msg),
						   "\"%s<%d><%s><%s>\" triggered \"chargeended\" (duration \"%.1f\")\n",
						   pInfo->GetName(), pInfo->GetUserID(),
						   pInfo->GetNetworkIDString(), szTeam,
						   flChargeDuration);
				engine->LogPrint(msg);
			}
		}

		if((iTeamNum == 2 && m_flRedMedicLostUberAdvantage > 0.0f) || (iTeamNum == 3 && m_flBlueMedicLostUberAdvantage > 0.0f))
		{
			if(mp_tournament.GetBool() && !tf_gamemode_mvm.GetBool())
			{
				IPlayerInfo *pInfo = playerinfomanager->GetPlayerInfo(gameents->BaseEntityToEdict(pPlayer));
				char szTeam[5] = "";

				if(iTeamNum == 2)
					V_strncpy(szTeam, "Red", sizeof(szTeam));
				else if(iTeamNum == 3)
					V_strncpy(szTeam, "Blue", sizeof(szTeam));

				char msg[128];

				V_snprintf(msg, sizeof(msg),
						   "\"%s<%d><%s><%s>\" triggered \"lost_uber_advantage\" (time \"%.0f\")\n",
						   pInfo->GetName(), pInfo->GetUserID(),
						   pInfo->GetNetworkIDString(), szTeam,
						   (iTeamNum == 2) ? m_flRedMedicLostUberAdvantage : m_flBlueMedicLostUberAdvantage);
				engine->LogPrint(msg);
			}
			((iTeamNum == 2) ? (m_flRedMedicLostUberAdvantage) : (m_flBlueMedicLostUberAdvantage)) = 0.0f;
		}
	}
}

void CLogs::LogAllHealing()
{
	if(gpGlobals->curtime - m_flHealingPrintTime > 1.0f)
	{
		for(int i = 0; i < 34; i++)
		{
			if(mapHealing[i].size() > 0)
			{
				for(auto const &j : mapHealing[i])
					LogHealing(i+1, j.first, j.second);

				mapHealing[i].clear();
			}
		}
		m_flHealingPrintTime = gpGlobals->curtime;
	}
}

void CLogs::OnServerActivate()
{
	m_flHealingPrintTime = 0.0f;
	m_flBlueMedicLastSpawnTime = 0.0f;
	m_flRedMedicLastSpawnTime = 0.0f;
	m_flBlueMedicHasUberChargeTime = 0.0f;
	m_flRedMedicHasUberChargeTime = 0.0f;
	m_flBlueUberChargeStartTime = 0.0f;
	m_flRedUberChargeStartTime = 0.0f;
	m_flBlueMedicLostUberAdvantage = 0.0f;
	m_flRedMedicLostUberAdvantage = 0.0f;

	memset(m_uiLastHealOnHit, 0, sizeof(m_uiLastHealOnHit));
	memset(m_uiLastDirectHitVictim, 0, sizeof(m_uiLastDirectHitVictim));
	m_flLastLogUploadTriedTime = 0.0f;
	m_iLastLogID = 0;
}

void CLogs::OnTournamentStarted()
{
	m_iLastLogID = 0;
}

void CLogs::LogHealing(int iHealer, int iPatient, int iAmount)
{
	edict_t *pPatientEdict = EdictFromIndex(iPatient);
	edict_t *pHealerEdict = EdictFromIndex(iHealer);

	if(pPatientEdict && pHealerEdict)
	{
		IPlayerInfo *pPatientInfo = playerinfomanager->GetPlayerInfo(pPatientEdict);
		IPlayerInfo *pHealerInfo = playerinfomanager->GetPlayerInfo(pHealerEdict);
		if(pPatientInfo && pHealerInfo)
		{
			char msg[512];
			char PatientTeam[5] = "";
			char HealerTeam[5] = "";
			int iPatientTeamNum = pPatientInfo->GetTeamIndex();
			int iHealerTeamNum = pHealerInfo->GetTeamIndex();

			float flMedicLastSpawnTime = 0.0f;

			if(iPatientTeamNum == 2)
				V_strncpy(PatientTeam,"Red",sizeof(PatientTeam));
			else if(iPatientTeamNum == 3)
				V_strncpy(PatientTeam,"Blue",sizeof(PatientTeam));
			if(iHealerTeamNum == 2)
			{
				V_strncpy(HealerTeam,"Red",sizeof(HealerTeam));
				flMedicLastSpawnTime = m_flRedMedicLastSpawnTime;
			}
			else if(iHealerTeamNum == 3)
			{
				V_strncpy(HealerTeam,"Blue",sizeof(HealerTeam));
				flMedicLastSpawnTime = m_flBlueMedicLastSpawnTime;
			}

			V_snprintf(msg, sizeof(msg),
					   "\"%s<%d><%s><%s>\" triggered \"healed\" against \"%s<%d><%s><%s>\" (healing \"%d\")\n",
					   pHealerInfo->GetName(), pHealerInfo->GetUserID(),
					   pHealerInfo->GetNetworkIDString(), HealerTeam,
					   pPatientInfo->GetName(), pPatientInfo->GetUserID(),
					   pPatientInfo->GetNetworkIDString(), PatientTeam, iAmount);
			engine->LogPrint(msg);

			if(flMedicLastSpawnTime > 0.0f && m_flLastRoundStart > 0.0f
					&& gpGlobals->curtime - m_flLastRoundStart >= 20.0f)
			{
				V_snprintf(msg, sizeof(msg),
						   "\"%s<%d><%s><%s>\" triggered \"first_heal_after_spawn\" (time \"%.1f\")\n",
						   pHealerInfo->GetName(), pHealerInfo->GetUserID(),
						   pHealerInfo->GetNetworkIDString(), HealerTeam,
						   gpGlobals->curtime - flMedicLastSpawnTime);
				engine->LogPrint(msg);
				if(iHealerTeamNum == 2)
					m_flRedMedicLastSpawnTime = 0.0f;
				else if(iHealerTeamNum == 3)
					m_flBlueMedicLastSpawnTime = 0.0f;
			}
		}
	}
}

void CLogs::FireGameEvent(IGameEvent *pEvent)
{
	static ConVarRef mp_tournament("mp_tournament");
	static ConVarRef tf_gamemode_mvm("tf_gamemode_mvm");

	if( !strcmp(pEvent->GetName(),"item_pickup"))
	{
		//	"userid"	"short"
		//	"item"		"string"
		int iUserId = pEvent->GetInt("userid");
		int iIndex = GetEntIndexFromUserID(iUserId);
		char szItem[64];
		V_strncpy(szItem, pEvent->GetString("item"), sizeof(szItem));

		if(iIndex != -1)
		{
			edict_t *pEdict = EdictFromIndex(iIndex);
			if(pEdict)
			{
				IPlayerInfo *pInfo = playerinfomanager->GetPlayerInfo(pEdict);
				if(pInfo)
				{
					char msg[256];
					int iTeamNum = pInfo->GetTeamIndex();
					char szTeamName[5];

					if(iTeamNum == 2)
						V_strncpy(szTeamName, "Red", sizeof(szTeamName));
					else if(iTeamNum == 3)
						V_strncpy(szTeamName, "Blue", sizeof(szTeamName));

					char szHealing[32] = "";
					if(strncmp(szItem, "medkit_", 7) == 0)
					{
						V_snprintf(szHealing, sizeof(szHealing),
								   " (healing \"%d\")", m_uiLastHealOnHit[iIndex-1]);
						m_uiLastHealOnHit[iIndex-1] = 0;
					}

					V_snprintf(msg,sizeof(msg),"\"%s<%d><%s><%s>\" picked up item \"%s\"%s\n",
							   pInfo->GetName(),
							   iUserId,
							   pInfo->GetNetworkIDString(),
							   szTeamName,
							   szItem,
							   szHealing);
					engine->LogPrint(msg);
				}
			}
		}
	}
	else if( !strcmp(pEvent->GetName(),"player_healed") && !tftrue_logs_includebuffs.GetBool() )
	{
		// This event logs normal heals and crossbow heals, but not buffs.

		//	"patient"	"short"
		//	"healer"	"short"
		//	"amount"	"short"
		int iPatientUserId = pEvent->GetInt("patient");
		int iPatientIndex = GetEntIndexFromUserID(iPatientUserId);
		int iHealerUserId = pEvent->GetInt("healer");
		int iHealerIndex = GetEntIndexFromUserID(iHealerUserId);
		int iHealAmount = pEvent->GetInt("amount");

		if((iPatientIndex != -1) && (iHealerIndex != -1))
			LogHealing(iHealerIndex, iPatientIndex, iHealAmount);
	}
	else if( !strcmp(pEvent->GetName(),"crossbow_heal") )
	{
		//	"target"	"short"
		//	"healer"	"short"
		//	"amount"	"short"

		// When you arrow a teammate, the "player_healonhit" event triggers
		// on the teammate, which stores heal amount in m_uiLastHealOnHit[teammate],
		// which will be logged as healing the next time they do damage.
		// This is wrong - it is not the teammate that caused the healing, but the medic.
		// Therefore, we set it back to zero here. 
		// It works because "crossbow_heal" is triggered after "player_healonhit".

		int iTargetUserId = pEvent->GetInt("target");
		int iTargetIndex = GetEntIndexFromUserID(iTargetUserId);
		m_uiLastHealOnHit[iTargetIndex-1] = 0;

		if (tftrue_logs_includebuffs.GetBool())
		{
			// When we include buffs, we no longer log the "player_healed" events,
			// so crossbow heals are not logged. Therefore we have to explicitly log
			// them using the "crossbow_heal" event.

			int iHealerUserId = pEvent->GetInt("healer");
			int iHealerIndex = GetEntIndexFromUserID(iHealerUserId);
			int iHealAmount = pEvent->GetInt("amount");

			if ((iTargetIndex != -1) && (iHealerIndex != -1))
				LogHealing(iHealerIndex, iTargetIndex, iHealAmount);
		}
	}
	else if( !strcmp(pEvent->GetName(),"player_spawn"))
	{
		//	"userid"	"short"
		//	"team"		"short"
		//	"class"		"short"
		int iUserId = pEvent->GetInt("userid");
		int iTeamNum = pEvent->GetInt("team");
		int iClass = pEvent->GetInt("class");
		int iIndex = GetEntIndexFromUserID(iUserId);

		const char *szClass[] = {"Scout", "Sniper", "Soldier", "Demoman", "Medic",
								 "Heavy", "Pyro", "Spy", "Engineer"};

		if(iIndex != -1)
		{
			edict_t *pEdict = EdictFromIndex(iIndex);
			if(pEdict)
			{
				IPlayerInfo *pInfo = playerinfomanager->GetPlayerInfo(pEdict);
				if(pInfo)
				{
					char msg[256];
					if(iTeamNum == 2)
					{
						V_snprintf(msg,sizeof(msg),"\"%s<%d><%s><Red>\" spawned as \"%s\"\n", pInfo->GetName(), iUserId, pInfo->GetNetworkIDString(), szClass[iClass-1]);
						engine->LogPrint(msg);
						if(iClass == TFCLASS_MEDIC && mp_tournament.GetBool() && !tf_gamemode_mvm.GetBool())
						{
							m_flRedMedicLastSpawnTime = gpGlobals->curtime;

							float flChargeLevel = GetChargeLevel((CBasePlayer*)CBaseEntity::Instance(pEdict));

							if(flChargeLevel == 0.0f)
							{
								m_flRedMedicHasUberChargeTime = 0.0f;
								m_flRedUberChargeStartTime = 0.0f;
								m_flRedMedicLostUberAdvantage = 0.0f;

								V_snprintf(msg ,sizeof(msg),
										   "\"%s<%d><%s><Red>\" triggered \"empty_uber\"\n", pInfo->GetName(), iUserId, pInfo->GetNetworkIDString());
								engine->LogPrint(msg);
							}
						}
					}
					else if(iTeamNum == 3)
					{
						V_snprintf(msg,sizeof(msg),"\"%s<%d><%s><Blue>\" spawned as \"%s\"\n", pInfo->GetName(), iUserId, pInfo->GetNetworkIDString(), szClass[iClass-1]);
						engine->LogPrint(msg);
						if(iClass == TFCLASS_MEDIC && mp_tournament.GetBool() && !tf_gamemode_mvm.GetBool())
						{
							m_flBlueMedicLastSpawnTime = gpGlobals->curtime;

							float flChargeLevel = GetChargeLevel((CBasePlayer*)CBaseEntity::Instance(pEdict));

							if(flChargeLevel == 0.0f)
							{
								m_flBlueMedicHasUberChargeTime = 0.0f;
								m_flBlueUberChargeStartTime = 0.0f;
								m_flBlueMedicLostUberAdvantage = 0.0f;

								V_snprintf(msg, sizeof(msg),
										   "\"%s<%d><%s><Blue>\" triggered \"empty_uber\"\n", pInfo->GetName(), iUserId, pInfo->GetNetworkIDString());
								engine->LogPrint(msg);
							}
						}
					}
				}
			}
		}
	}
	else if( !strcmp(pEvent->GetName(), "player_chargedeployed"))
	{
		int iUserId = pEvent->GetInt("userid");
		int iIndex = GetEntIndexFromUserID(iUserId);

		if(iIndex != -1)
		{
			edict_t *pEdict = EdictFromIndex(iIndex);
			if(pEdict)
			{
				IPlayerInfo *pInfo = playerinfomanager->GetPlayerInfo(pEdict);

				char msg[256];
				int iTeamNum = pInfo->GetTeamIndex();
				char szTeamName[5];

				if(iTeamNum == 2)
				{
					V_strncpy(szTeamName, "Red", sizeof(szTeamName));
					m_flRedUberChargeStartTime = gpGlobals->curtime;
					m_flRedMedicLostUberAdvantage = 0.0f;
				}
				else if(iTeamNum == 3)
				{
					V_strncpy(szTeamName, "Blue", sizeof(szTeamName));
					m_flBlueUberChargeStartTime = gpGlobals->curtime;
					m_flBlueMedicLostUberAdvantage = 0.0f;
				}
				char szMedigun[20];

				// This is probably bad, +4 doesn't mean slot2, it *could* be another weapon than medigun
				CBaseEntity *pMedigun = *(CBaseCombatWeaponHandle*)(g_EntityProps.GetSendProp<char>(CBaseEntity::Instance(pEdict), "m_hMyWeapons")+4);

				if(pMedigun)
				{
					switch(*g_EntityProps.GetSendProp<int>(pMedigun, "m_AttributeManager.m_Item.m_iItemDefinitionIndex"))
					{
						case 29:           // Medi Gun
						case 211:          // Medi Gun(Renamed/Strange)
						case 663:          // Festive Medi Gun
						case 796:          // Silver Botkiller Medi Gun Mk.I
						case 805:          // Gold Botkiller Medi Gun Mk.I
						case 885:          // Rust Botkiller Medi Gun Mk.I
						case 894:          // Blood Botkiller Medi Gun Mk.I
						case 903:          // Carbonado Botkiller Medi Gun Mk.I
						case 912:          // Diamond Botkiller Medi Gun Mk.I
						case 961:          // Silver Botkiller Medi Gun Mk.II
						case 970:          // Gold Botkiller Medi Gun Mk.II
						case 15008:        // Masked Mender
						case 15010:        // Wrapped Reviver
						case 15025:        // Reclaimed Reanimator
						case 15039:        // Civil Servant
						case 15050:        // Spark of Life
						case 15078:        // Wildwood
						case 15097:        // Flower Power
						case 15120:        // Coffin Nail
						case 15121:        // Dressed To Kill
						case 15122:        // High Roller's
						case 15145:        // Blitzkrieg
						case 15146:        // Corsair
						case 16001:        // template_grade_medigun
						case 16013:        // template_grade_medigun
						case 16024:        // template_grade_medigun
							V_strncpy(szMedigun, "medigun", sizeof(szMedigun));
							break;
						case 35:
							V_strncpy(szMedigun, "kritzkrieg", sizeof(szMedigun));
							break;
						case 411:
							V_strncpy(szMedigun, "quickfix", sizeof(szMedigun));
							break;
						case 998:
							V_strncpy(szMedigun, "vaccinator", sizeof(szMedigun));
							break;
						default:
							V_strncpy(szMedigun, "unknown", sizeof(szMedigun));
							break;
					}

					V_snprintf(msg, sizeof(msg), "\"%s<%d><%s><%s>\" triggered \"chargedeployed\" (medigun \"%s\")\n",
							   pInfo->GetName(),
							   iUserId,
							   pInfo->GetNetworkIDString(),
							   szTeamName,
							   szMedigun);

					engine->LogPrint(msg);
				}
			}
		}
	}
	else if( !strcmp(pEvent->GetName(), "player_healonhit"))
	{
		int iEntIndex = pEvent->GetInt("entindex");
		int iAmount = pEvent->GetInt("amount");
		m_uiLastHealOnHit[iEntIndex-1] = iAmount;
	}
	else if( !strcmp(pEvent->GetName(), "teamplay_round_start"))
	{
		m_flLastRoundStart = gpGlobals->curtime;
	}
	else if ( !strcmp(pEvent->GetName(), "projectile_direct_hit"))
	{
		int iAttacker = pEvent->GetInt("attacker");
		int iVictim = pEvent->GetInt("victim");
		if (iVictim > 0)
		{
			m_uiLastDirectHitVictim[iAttacker-1] = iVictim;
		}
	}
}

void CLogs::Event_PlayerFiredWeapon( void *pTFGameStats, EDX CBasePlayer *pPlayer, bool bCritical )
{
	if(tftrue_logs_accuracy.GetBool())
	{
		edict_t *pEdictAttacker = gameents->BaseEntityToEdict(pPlayer);

		CBaseEntity *pWeapon = *g_EntityProps.GetSendProp<EHANDLE>(pPlayer, "m_hActiveWeapon");
		if(pWeapon)
		{
			int iDefIndex = *g_EntityProps.GetSendProp<int>(pWeapon, "m_AttributeManager.m_Item.m_iItemDefinitionIndex");

			const char *szItemLogName = g_Items.GetItemLogName(iDefIndex);

			char szWeaponName[32] = "";

			if(!szItemLogName)
			{
				const char *pClassName = (char*)pWeapon->GetClassname();
				if(strncmp(pClassName, "tf_weapon_", 10) == 0)
				{
					V_strncpy(szWeaponName, (char*)((unsigned long)pClassName+10), sizeof(szWeaponName));

					if (strcmp("rocketlauncher", szWeaponName) == 0)
						V_strncpy(szWeaponName, "tf_projectile_rocket", sizeof(szWeaponName));
					else if (strcmp("pipebomblauncher", szWeaponName) == 0)
						V_strncpy(szWeaponName, "tf_projectile_pipe_remote", sizeof(szWeaponName));
					else if (strcmp("grenadelauncher", szWeaponName) == 0)
						V_strncpy(szWeaponName, "tf_projectile_pipe", sizeof(szWeaponName));
				}
			}
			else
				V_strncpy(szWeaponName, szItemLogName, sizeof(szWeaponName));

			IPlayerInfo *pInfoAttacker = playerinfomanager->GetPlayerInfo(pEdictAttacker);
			if(pInfoAttacker)
			{
				char msg[256];
				char szTeamNameAttacker[5];

				int iTeamNumAttacker = pInfoAttacker->GetTeamIndex();

				if(iTeamNumAttacker == 2)
					V_strncpy(szTeamNameAttacker, "Red", sizeof(szTeamNameAttacker));
				else if(iTeamNumAttacker == 3)
					V_strncpy(szTeamNameAttacker, "Blue", sizeof(szTeamNameAttacker));

				V_snprintf(msg, sizeof(msg), "\"%s<%d><%s><%s>\" triggered \"shot_fired\" (weapon \"%s\")\n",
						   pInfoAttacker->GetName(),
						   pInfoAttacker->GetUserID(),
						   pInfoAttacker->GetNetworkIDString(),
						   szTeamNameAttacker,
						   szWeaponName);
				engine->LogPrint(msg);
			}
		}
	}

	g_Logs.Event_PlayerFiredWeaponRoute.CallOriginalFunction<Event_PlayerFiredWeapon_t>()(pTFGameStats, pPlayer, bCritical);
}

void CLogs::Event_PlayerHealedOther( void *pTFGameStats, EDX CBasePlayer *pPlayer, float flHealing )
{
	if(*g_EntityProps.GetSendProp<int>(pPlayer, "m_PlayerClass.m_iClass") == TFCLASS_MEDIC && tftrue_logs_includebuffs.GetBool())
	{
		// This is probably bad, +4 doesn't mean slot2, it *could* be another weapon than medigun
		CBaseEntity *pMedigun = *(CBaseCombatWeaponHandle*)(g_EntityProps.GetSendProp<char>(pPlayer, "m_hMyWeapons")+4);
		CBaseEntity *pPatient = NULL;

		if(pMedigun)
			pPatient = *g_EntityProps.GetSendProp<EHANDLE>(pMedigun, "m_hHealingTarget");

		if(pPatient)
		{
			int iHealerIndex = pPlayer->entindex();
			int iPatientIndex = pPatient->entindex();

			g_Logs.mapHealing[iHealerIndex-1][iPatientIndex] += (int)flHealing;
		}
	}

	g_Logs.Event_PlayerHealedOtherRoute.CallOriginalFunction<Event_PlayerHealedOther_t>()(pTFGameStats, pPlayer, flHealing);
}

int CLogs::OnTakeDamage( CBasePlayer *pPlayer, EDX const CTakeDamageInfo &info )
{
	float flChargeLevel = 0.0f;

	if(*g_EntityProps.GetSendProp<int>(pPlayer, "m_PlayerClass.m_iClass") == TFCLASS_MEDIC)
		flChargeLevel = g_Logs.GetChargeLevel(pPlayer);

	int iResult = g_Logs.OnTakeDamageRoute.CallOriginalFunction<OnTakeDamage_t>()(pPlayer, info);

	if(iResult == 0)
		return iResult;

	CBasePlayer *pAttacker = (CBasePlayer*)info.GetAttacker();

	if(pPlayer != pAttacker && IndexOfEntity(pAttacker) > 0 && IndexOfEntity(pAttacker) <= g_pServer->GetClientCount())
	{
		edict_t *pEdictVictim = gameents->BaseEntityToEdict(pPlayer);

		int iVictimHealth = *g_EntityProps.GetSendProp<int>(pPlayer, "m_iHealth");
		IPlayerInfo *pInfoVictim = playerinfomanager->GetPlayerInfo(pEdictVictim);

		if(pInfoVictim)
		{
			if(iVictimHealth <= 0 && *g_EntityProps.GetSendProp<int>(pPlayer, "m_PlayerClass.m_iClass") == TFCLASS_MEDIC)
			{
				char msg[256];
				char szTeamNameVictim[5];

				int iTeamNumVictim = pInfoVictim->GetTeamIndex();

				if(iTeamNumVictim == 2)
					V_strncpy(szTeamNameVictim, "Red", sizeof(szTeamNameVictim));
				else if(iTeamNumVictim == 3)
					V_strncpy(szTeamNameVictim, "Blue", sizeof(szTeamNameVictim));

				V_snprintf(msg, sizeof(msg), "\"%s<%d><%s><%s>\" triggered \"medic_death_ex\" (uberpct \"%.0f\")\n",
						   pInfoVictim->GetName(),
						   pInfoVictim->GetUserID(),
						   pInfoVictim->GetNetworkIDString(),
						   szTeamNameVictim,
						   flChargeLevel);

				engine->LogPrint(msg);
			}
		}
	}

	return iResult;
}

void CLogs::Event_PlayerDamage( void *pTFGameStats, EDX CBasePlayer *pPlayer, const CTakeDamageInfo &info, int iDamageTaken )
{
	CBasePlayer *pAttacker = (CBasePlayer*)info.GetAttacker();

	if(pPlayer != pAttacker && IndexOfEntity(pAttacker) > 0 && IndexOfEntity(pAttacker) <= g_pServer->GetClientCount())
	{
		edict_t *pEdictAttacker = gameents->BaseEntityToEdict(pAttacker);
		edict_t *pEdictVictim = gameents->BaseEntityToEdict(pPlayer);

		bool bCrit = (info.GetDamageType() & DMG_BURN) != 0;
		bool bMiniCrit = *(bool*)(g_EntityProps.GetSendProp<char>(pPlayer, "m_angEyeAngles[0]")-34);

		int iWeaponID = 0;
		const char *pWeaponName = reinterpret_cast<GetKillingWeaponName_t>(g_Logs.GetKillingWeaponName)( g_Plugin.GetGameRules(), info, pPlayer, &iWeaponID );

		CBaseEntity *pWeapon = info.GetWeapon();
		if(pWeapon)
		{
			int iDefIndex = *g_EntityProps.GetSendProp<int>(pWeapon, "m_AttributeManager.m_Item.m_iItemDefinitionIndex");

			const char* pWeaponLogName = g_Items.GetItemLogName(iDefIndex);
			if(pWeaponLogName)
				pWeaponName = pWeaponLogName;
			else if(!pWeaponName)
			{
				pWeaponName = (char*)pWeapon->GetClassname();
				if(strncmp(pWeaponName, "tf_weapon_", 10) == 0)
					pWeaponName = (char*)((unsigned long)pWeaponName+10);
			}
		}

		IPlayerInfo *pInfoAttacker = playerinfomanager->GetPlayerInfo(pEdictAttacker);
		IPlayerInfo *pInfoVictim = playerinfomanager->GetPlayerInfo(pEdictVictim);

		if(pInfoAttacker && pInfoVictim)
		{
			char msg[256];
			char szTeamNameAttacker[5];
			char szTeamNameVictim[5];

			int iTeamNumAttacker = pInfoAttacker->GetTeamIndex();
			int iTeamNumVictim = pInfoVictim->GetTeamIndex();

			if(iTeamNumAttacker == 2)
				V_strncpy(szTeamNameAttacker, "Red", sizeof(szTeamNameAttacker));
			else if(iTeamNumAttacker == 3)
				V_strncpy(szTeamNameAttacker, "Blue", sizeof(szTeamNameAttacker));

			if(iTeamNumVictim == 2)
				V_strncpy(szTeamNameVictim, "Red", sizeof(szTeamNameVictim));
			else if(iTeamNumVictim == 3)
				V_strncpy(szTeamNameVictim, "Blue", sizeof(szTeamNameVictim));

			char szRealDamage[32] = "";
			char szCrit[32] = "";
			char szAirshot[32] = "";
			char szDirect[32] = "";
			char szHeadshot[32] = "";
			char szHealing[32] = "";
			char szAirshotHeight[32] = "";

			if(iDamageTaken != (int)(info.GetDamage()+0.5f))
				V_snprintf(szRealDamage, sizeof(szRealDamage), " (realdamage \"%d\")", iDamageTaken);

			if(bCrit)
				V_strncpy(szCrit, " (crit \"crit\")", sizeof(szCrit));
			else if(bMiniCrit)
				V_strncpy(szCrit, " (crit \"minicrit\")", sizeof(szCrit));

			if (info.GetInflictor() && (!strcmp(info.GetInflictor()->GetClassname(), "tf_projectile_rocket") || !strcmp(info.GetInflictor()->GetClassname(), "tf_projectile_energy_ball") || !strcmp(info.GetInflictor()->GetClassname(), "tf_projectile_pipe") || !strcmp(info.GetInflictor()->GetClassname(), "tf_projectile_healing_bolt")))
			{
				if(!(*g_EntityProps.GetDataMapProp<int>(pPlayer, "m_fFlags") & (FL_ONGROUND|FL_INWATER)))
				{
					Ray_t pRay;
					trace_t trace;
					Vector vAimerOrigin = pInfoVictim->GetAbsOrigin();
					Vector vDirection;
					Vector vhullMins;
					Vector vhullMaxs;

					AngleVectors(QAngle(90, 0, 0), &vDirection);
					vDirection = vDirection * 8192 + vAimerOrigin;
					// Values represent the area of a tf2 players bbox see: https://developer.valvesoftware.com/wiki/TF2/Team_Fortress_2_Mapper%27s_Reference
					// Values are rounded down to allow for some leeway 
					vhullMins.Init(24,24,0);
					vhullMaxs.Init(-24,-24,0);

					pRay.Init(vAimerOrigin,vDirection, vhullMins, vhullMaxs);
					CTraceFilterWorldOnly traceFilter;

					g_pEngineTrace->TraceRay(pRay, CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_HITBOX|CONTENTS_GRATE, &traceFilter, &trace);

					if(trace.DidHit())
					{
						// Calculate height using z difference instead of DistTo since using a ray with hulls introduces some x,y difference
						float distance = abs(vAimerOrigin.z - trace.endpos.z);
						if(distance >= 170.0)
						{
							V_strncpy(szAirshot, " (airshot \"1\")", sizeof(szAirshot));
							V_snprintf(szAirshotHeight, sizeof(szAirshotHeight), " (height \"%d\")",(int) distance);
						}
					}
				}
				// If the attacker hit the victim with a direct hit. For pipes a workaround(m_bTouched) is needed since the event "projectile_direct_hit" gets triggered after the damage
				if (g_Logs.m_uiLastDirectHitVictim[IndexOfEdict(pEdictAttacker)] == IndexOfEdict(pEdictVictim) || !*g_EntityProps.GetSendProp<bool>(info.GetInflictor(), "m_bTouched"))
				{
					V_strncpy(szDirect, " (direct \"1\")", sizeof(szDirect));
				}
				g_Logs.m_uiLastDirectHitVictim[IndexOfEdict(pEdictAttacker)] = 0;
			}

			if(info.GetDamageCustom() == TF_CUSTOM_HEADSHOT || info.GetDamageCustom() == TF_CUSTOM_HEADSHOT_DECAPITATION)
				V_strncpy(szHeadshot, " (headshot \"1\")", sizeof(szHeadshot));

			unsigned int uiHealing = g_Logs.m_uiLastHealOnHit[IndexOfEdict(pEdictAttacker)-1];

			if(uiHealing > 0) {
				V_snprintf(szHealing, sizeof(szHealing),
						   " (healing \"%d\")", uiHealing);
				// Make sure to reset the last heal on hit value
				g_Logs.m_uiLastHealOnHit[IndexOfEdict(pEdictAttacker)-1] = 0;
			}

			V_snprintf(msg, sizeof(msg), "\"%s<%d><%s><%s>\" triggered \"damage\" against \"%s<%d><%s><%s>\" (damage \"%d\")%s (weapon \"%s\")%s%s%s%s%s%s\n",
					   pInfoAttacker->GetName(),
					   pInfoAttacker->GetUserID(),
					   pInfoAttacker->GetNetworkIDString(),
					   szTeamNameAttacker,
					   pInfoVictim->GetName(),
					   pInfoVictim->GetUserID(),
					   pInfoVictim->GetNetworkIDString(),
					   szTeamNameVictim,
					   (int)(info.GetDamage()+0.5f),
					   szRealDamage,
					   pWeaponName,
					   szHealing,
					   szCrit,
					   szAirshot,
					   szAirshotHeight,
					   szDirect,
					   szHeadshot);
			engine->LogPrint(msg);

			if(tftrue_logs_accuracy.GetBool())
			{
				V_snprintf(msg, sizeof(msg), "\"%s<%d><%s><%s>\" triggered \"shot_hit\" (weapon \"%s\")\n",
						   pInfoAttacker->GetName(),
						   pInfoAttacker->GetUserID(),
						   pInfoAttacker->GetNetworkIDString(),
						   szTeamNameAttacker,
						   pWeaponName);
				engine->LogPrint(msg);
			}
		}
	}

	g_Logs.Event_PlayerDamageRoute.CallOriginalFunction<Event_PlayerDamage_t>()(pTFGameStats, pPlayer, info, iDamageTaken);
}

void CLogs::LogPrint( IVEngineServer *pServer, EDX const char *msg )
{
	// Prevent logging the normal chargedeployed, without the medigun
	const char *pChargeDeployed = strstr(msg, ">\" triggered \"chargedeployed\"\n");
	if(pChargeDeployed && strlen(pChargeDeployed) == 30)
	{
		if(!strstr(msg, ">\" say \"")) // Make sure the player didn't try to fake it
			return;
	}

	g_Logs.LogPrintRoute.CallOriginalFunction<LogPrint_t>()(pServer, msg);
}

void CLogs::ApiKey_Callback( IConVar *var, const char *pOldValue, float flOldValue )
{
	ConVar* v = (ConVar*)var;

	// Don't check if there is already a log, otherwise we might not know its name
	// if we have just auto updated
	if(strcmp(v->GetString(), "") && !strcmp(g_Logs.m_szCurrentLogFile, ""))
	{
		engine->InsertServerCommand("log on\n");
		engine->ServerExecute();
	}
}

void CLogs::OnLogCommand()
{
	if(strcmp(m_szLastUploadedLog, ""))
		ShowHTMLMOTD(g_Plugin.GetCommandIndex()+1, "Match log", m_szLastUploadedLog);
	else
		Message(g_Plugin.GetCommandIndex()+1, "\003No log has been uploaded so far.");
}

float CLogs::GetChargeLevel(CBasePlayer *pPlayer)
{
	float flChargeLevel = 0.0f;

	// This is probably bad, +4 doesn't mean slot2, it *could* be another weapon than medigun
	CBaseEntity *pMedigun = *(CBaseCombatWeaponHandle*)(g_EntityProps.GetSendProp<char>(pPlayer, "m_hMyWeapons")+4);
	if(pMedigun)
		flChargeLevel = *g_EntityProps.GetSendProp<float>(pMedigun, "m_flChargeLevel")*100;
	return flChargeLevel;
}

void CLogs::Upload(bool bRoundEnd)
{
	// Calculate a boundary
	char szBoundary[41];
	RandomSeed(time(NULL));
	V_snprintf(szBoundary, sizeof(szBoundary), "------------------TFTrue%08x%08x", RandomInt(0, INT_MAX), RandomInt(0, INT_MAX));

	FILE *logFile = fopen(m_szCurrentLogFileBackup, "rb");
	if(!logFile)
	{
		AllMessage("\003[TFTrue] Failed to open the log file\n");
		Msg("[TFTrue] Failed to open the log file\n");
		engine->LogPrint("[TFTrue] Failed to open the log file\n");
		return;
	}

	// Start of the actual content, we need its size to find out the Content-Length
	fseek(logFile, 0L, SEEK_END);
	long lFileLength = ftell(logFile);
	fseek(logFile, 0L, SEEK_SET);

	static ConVarRef mp_tournament_blueteamname("mp_tournament_blueteamname");
	static ConVarRef mp_tournament_redteamname("mp_tournament_redteamname");

	// Create the form data before the header to be able to fill Content-Length
	std::string strContent;
	strContent.reserve(1024);

	// Add log title
	strContent.append("--").append(szBoundary).append("\r\nContent-Disposition: form-data; name=\"title\"\r\n\r\n");
	if(strcmp(tftrue_logs_name_prefix.GetString(), ""))
		strContent.append(tftrue_logs_name_prefix.GetString()).append(" - ");
	strContent.append(mp_tournament_blueteamname.GetString()).append(" vs ").append(mp_tournament_redteamname.GetString()).append("\r\n");

	// Add map name
	strContent.append("--").append(szBoundary).append("\r\nContent-Disposition: form-data; name=\"map\"\r\n\r\n").append(gpGlobals->mapname.ToCStr()).append("\r\n");

	// Add api key
	strContent.append("--").append(szBoundary).append("\r\nContent-Disposition: form-data; name=\"key\"\r\n\r\n").append(tftrue_logs_apikey.GetString()).append("\r\n");

	// Add plugin name
	strContent.append("--").append(szBoundary).append("\r\nContent-Disposition: form-data; name=\"uploader\"\r\n\r\nTFTrue v").append(tftrue_version.GetString()).append("\r\n");

	// Add updatelog
	if(m_iLastLogID != 0)
		strContent.append("--").append(szBoundary).append("\r\nContent-Disposition: form-data; name=\"updatelog\"\r\n\r\n").append(std::to_string(m_iLastLogID)).append("\r\n");

	// Add log file
	strContent.append("--").append(szBoundary).append("\r\nContent-Disposition: form-data; name=\"logfile\"; filename=\"TFTrue.log\"\r\nContent-Type: application/octet-stream\r\n\r\n");

	char szContent2[1024];
	size_t numBytesRead = 0;
	size_t totalBytesRead = 0;

	while((numBytesRead = fread(szContent2, 1, sizeof(szContent2), logFile)) > 0)
	{
		totalBytesRead += numBytesRead;

		// If the game wrote to the log after we got the file size
		// we might send too much data, so we need to remove the extra bytes
		if(totalBytesRead > lFileLength)
			numBytesRead -= (totalBytesRead - lFileLength);

		strContent.append(szContent2);
	}

	fclose(logFile);

	char szContentEnd[50];
	V_snprintf(szContentEnd, sizeof(szContentEnd), "\r\n--%s--\r\n", szBoundary);
	strContent.append(szContentEnd);

	std::string contentType = "multipart/form-data; boundary=";
	contentType.append(szBoundary);

	std::unique_ptr<LogUploadRequest> logUploadRequest(new LogUploadRequest());
	SteamAPICall_t hCallServer;
	logUploadRequest->handle = steam.SteamHTTP()->CreateHTTPRequest(k_EHTTPMethodPOST, "https://logs.tf/upload");
	steam.SteamHTTP()->SetHTTPRequestHeaderValue(logUploadRequest->handle, "Cache-Control", "no-cache");
	steam.SteamHTTP()->SetHTTPRequestRawPostBody(logUploadRequest->handle, contentType.c_str(), (uint8*)strContent.c_str(), strContent.size());
	steam.SteamHTTP()->SendHTTPRequest(logUploadRequest->handle, &hCallServer);

	logUploadRequest->callResult.SetGameserverFlag();
	logUploadRequest->callResult.Set(hCallServer, &g_Logs, &CLogs::UploadLogCallback);
	logUploadRequest->roundEnd = bRoundEnd;

	m_LogUploadRequests.emplace_back(std::move(logUploadRequest));

}

void CLogs::UploadLogCallback(HTTPRequestCompleted_t *arg, bool bFailed)
{
	std::vector<std::unique_ptr<LogUploadRequest>>::iterator currLogUploadRequest;
	for ( auto it = m_LogUploadRequests.begin(); it != m_LogUploadRequests.end(); it++ )
	{
		if(it->get()->handle == arg->m_hRequest)
		{
			currLogUploadRequest = it;
			break;
		}
	}

	if(bFailed || arg->m_eStatusCode < 200 || arg->m_eStatusCode > 299)
	{
		uint32 size;
		steam.SteamHTTP()->GetHTTPResponseBodySize(arg->m_hRequest, &size);

		if(size > 0)
		{
			uint8 *pResponse = new uint8[size+1];
			steam.SteamHTTP()->GetHTTPResponseBodyData(arg->m_hRequest, pResponse, size);
			pResponse[size] = '\0';

			Msg("[TFTrue] The log hasn't been uploaded. HTTP error %d. Response: %s\n", arg->m_eStatusCode, pResponse);

			delete[] pResponse;
		}
		else if(!arg->m_bRequestSuccessful)
		{
			Msg("[TFTrue] The log hasn't been received. No response from the server.\n");
		}
		else
		{
			Msg("[TFTrue] The log hasn't been received. HTTP error %d\n", arg->m_eStatusCode);
		}
	}
	else
	{
		uint32 size;
		steam.SteamHTTP()->GetHTTPResponseBodySize(arg->m_hRequest, &size);

		if(size > 0)
		{
			uint8 *pResponse = new uint8[size];
			steam.SteamHTTP()->GetHTTPResponseBodyData(arg->m_hRequest, pResponse, size);

			Json::Value root;   // will contains the root value after parsing.
			Json::Reader reader;

			bool parsingSuccessful = reader.parse((const char*)pResponse, (const char*)&pResponse[size], root);
			if ( parsingSuccessful )
			{
				Json::Value success = root.get("success", false);
				bool bSuccess = success.asBool();

				if(bSuccess)
				{
					Json::Value log_id = root.get("log_id", 0).asInt();
					int iLogId = log_id.asInt();

					V_snprintf(m_szLastUploadedLog, sizeof(m_szLastUploadedLog), "http://logs.tf/%d", iLogId);

					if(currLogUploadRequest->get()->roundEnd)
						m_iLastLogID = iLogId;
					else
						m_iLastLogID = 0;

					char Line[255];
					sprintf(Line, "\003[TFTrue] The log is available here: %s. Type !log to view it.\n", m_szLastUploadedLog);
					AllMessage(Line);
					Msg("[TFTrue] The log is available here: %s\n", m_szLastUploadedLog);
					engine->LogPrint(Line+1);
				}
				else
				{
					Json::Value error = root.get("error", "Unknown");
					const char* cszError = error.asCString();

					char Line[255];
					sprintf(Line, "\003[TFTrue] Failed to upload the log: %s\n", cszError);
					AllMessage(Line);
					Msg("[TFTrue] Failed to upload the log: %s\n", cszError);
					engine->LogPrint(Line+1);
				}
			}
			else
			{
				// report to the user the failure and their locations in the document.
				AllMessage("\003[TFTrue] Failed to parse json\n");
				Msg("[TFTrue] Failed to parse json\n");
				engine->LogPrint("[TFTrue] Failed to parse json\n");
			}

			delete[] pResponse;
		}
	}

	m_LogUploadRequests.erase(currLogUploadRequest);
	steam.SteamHTTP()->ReleaseHTTPRequest(arg->m_hRequest);
}

FileHandle_t CLogs::OpenEx( IFileSystem *pFileSystem, EDX const char *pFileName, const char *pOptions, unsigned flags, const char *pathID, char **ppszResolvedFilename )
{
	if(pathID && !strcmp(pathID, "LOGDIR") && pOptions && !strcmp(pOptions, "wt"))
	{
		char szPath[MAX_PATH];
		engine->GetGameDir(szPath, sizeof(szPath));

		char szFilePath[MAX_PATH];
		V_snprintf(szFilePath, sizeof(szFilePath), "%s/%s", szPath, pFileName);
		V_strncpy(g_Logs.m_szCurrentLogFile, szFilePath, sizeof(g_Logs.m_szCurrentLogFile));
		#ifdef _LINUX
		struct stat fileStat;

		// Sometimes the l is capitalized and it shouldn't
		if(stat(g_Logs.m_szCurrentLogFile, &fileStat) != 0)
		{
			char* fileName = basename(g_Logs.m_szCurrentLogFile);
			if(fileName[0] == 'L')
				fileName[0] = 'l';
		}
		#endif
	}

	return g_Logs.OpenExRoute.CallOriginalFunction<OpenEx_t>()(pFileSystem, pFileName, pOptions, flags, pathID, ppszResolvedFilename);
}
