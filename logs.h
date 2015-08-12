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

class CLog
{
public:
	void					*pVTable;
	bool					m_bActive;
	CUtlVector< netadr_t >	m_LogAddresses;
	FileHandle_t			m_hLogFile;
	double					m_flLastLogFlush;
	bool					m_bFlushLog;
};

class CLogs: public IGameEventListener2
{
public:
	CLogs();

	bool Init(const CModuleScanner& EngineModule, const CModuleScanner& ServerModule);

	void OnGameFrame();
	void OnUnload();
	void OnDisconnect(edict_t *pEntity);
	void OnServerActivate();
	void OnGameOver();
	void OnRoundWin();
	void OnTournamentStarted();
	void OnLogCommand();

	void LogAllHealing();
	void LogHealing(int iHealer, int iPatient, int iAmount);
	void LogUberchargeStatus();
	void UpdateLogState();
	float GetChargeLevel(CBasePlayer *pPlayer);
	void ResetLastLogID() { m_iLastLogID = 0; }

	virtual void FireGameEvent(IGameEvent* pEvent );

	static void ApiKey_Callback( IConVar *var, const char *pOldValue, float flOldValue );

private:
	std::map<int, int> mapHealing[34];

	float m_flHealingPrintTime = 0.0f;

	float m_flBlueMedicLastSpawnTime = 0.0f;
	float m_flRedMedicLastSpawnTime = 0.0f;

	float m_flBlueMedicHasUberChargeTime = 0.0f;
	float m_flRedMedicHasUberChargeTime = 0.0f;

	float m_flBlueUberChargeStartTime = 0.0f;
	float m_flRedUberChargeStartTime = 0.0f;

	float m_flBlueMedicLostUberAdvantage = 0.0f;
	float m_flRedMedicLostUberAdvantage = 0.0f;

	float m_flLastRoundStart = 0.0f;

	unsigned int m_uiLastHealOnHit[34];

	static void __fastcall Event_PlayerHealedOther( void *pTFGameStats, EDX CBasePlayer *pPlayer, float flHealing );
	static void __fastcall Event_PlayerFiredWeapon( void *pTFGameStats, EDX CBasePlayer *pPlayer, bool bCritical );
	static void __fastcall Event_PlayerDamage( void *pTFGameStats, EDX CBasePlayer *pPlayer, const CTakeDamageInfo &info, int iDamageTaken );
	static int __fastcall OnTakeDamage( CBasePlayer *pPlayer, EDX const CTakeDamageInfo &info );
	static void __fastcall LogPrint( IVEngineServer *pServer, EDX const char *msg );
	static FileHandle_t	__fastcall OpenEx( IFileSystem *pFileSystem, EDX const char *pFileName, const char *pOptions, unsigned flags, const char *pathID, char **ppszResolvedFilename);

	void *GetKillingWeaponName = nullptr;

	CFunctionRoute Event_PlayerHealedOtherRoute;
	CFunctionRoute Event_PlayerFiredWeaponRoute;
	CFunctionRoute Event_PlayerDamageRoute;
	CFunctionRoute OnTakeDamageRoute;
	CFunctionRoute LogPrintRoute;
	CFunctionRoute OpenExRoute;

	typedef void (__thiscall *Event_PlayerHealedOther_t)( void *thisPtr, CBasePlayer *pPlayer, float flHealing);
	typedef void (__thiscall *Event_PlayerFiredWeapon_t)( void *thisPtr, CBasePlayer *pPlayer, bool bCritical );
	typedef void (__thiscall *Event_PlayerDamage_t)( void *thisPtr, CBasePlayer *pPlayer, const CTakeDamageInfo &info, int iDamageTaken );
	typedef int (__thiscall *OnTakeDamage_t)( void *thisPtr, const CTakeDamageInfo &info );
	typedef const char* (__thiscall* GetKillingWeaponName_t)( void *thisPtr, const CTakeDamageInfo &info, CBasePlayer *pVictim, int *iWeaponID );
	typedef void (__thiscall *LogPrint_t)( void *pServer, const char* msg);
	typedef FileHandle_t (__thiscall *OpenEx_t)( IFileSystem *pFileSystem, const char *pFileName, const char *pOptions, unsigned flags, const char *pathID, char **ppszResolvedFilename);

	void Upload(bool bRoundEnd);

	float m_flLastLogUploadTriedTime = 0.0f;
	char m_szCurrentLogFile[MAX_PATH];
	char m_szCurrentLogFileBackup[MAX_PATH];
	char m_szLastUploadedLog[50];

	int m_iLastLogID = 0;

	enum eLogStates
	{
		STATE_NONE = 0,
		STATE_CREATENEW,
		STATE_FLUSH
	};

	eLogStates m_iLogState = STATE_NONE; // 0: Don't do anything, 1: Needs to create new log, 2: Needs to flush current log
};

extern CLogs g_Logs;

extern ConVar tftrue_logs_apikey;
extern ConVar tftrue_logs_name_prefix;
extern ConVar tftrue_logs_includebuffs;
extern ConVar tftrue_logs_accuracy;
extern ConVar tftrue_logs_roundend;
