#pragma once

#include "SDK.h"
#include "utils.h"

#include "ModuleScanner.h"

class CTournament: public IGameEventListener2
{
public:
	bool Init(const CModuleScanner& EngineModule, const CModuleScanner& ServerModule);
	void OnServerActivate();
	void OnUnload();

	bool BothTeamsReady() { return m_bRedReady && m_bBluReady; }
	void SetTournamentMapVars();
	void FindMapType();
	void DownloadConfig(const char *szURL, SOCKET sock, bool bOverwrite = true);

	void FireGameEvent(IGameEvent *pEvent );

	static void GameMode_Callback( IConVar *var, const char *pOldValue, float flOldValue );
	static void Tournament_Callback( IConVar *var, const char *pOldValue, float flOldValue );
	static void Tournament_Config_Callback( IConVar *var, const char *pOldValue, float flOldValue );

	bool OnDispatchCommand(ConCommand *pCmd, const CCommand &args);
	void Tournament_Restart_Callback(ConCommand *pCmd, const CCommand &args);
	void Pure_Callback(ConCommand *pCmd, const CCommand &args);
	void Status_Callback(ConCommand *pCmd, const CCommand &args);
	void Pause_Callback(ConCommand *pCmd, const CCommand &args);

	enum GameConfig
	{
		CONFIG_NONE,
		CONFIG_ETF2L6v6,
		CONFIG_ETF2L9v9
	};

private:
	bool m_bRedReady = false;
	bool m_bBluReady = false;

	int m_iConfigDownloadFailed = 0;

	enum MapType
	{
		MAPTYPE_UNKNOWN,
		MAPTYPE_ATTACKDEFENSE,
		MAPTYPE_CP,
		MAPTYPE_CTF,
		MAPTYPE_ARENA,
		MAPTYPE_MVM
	};

	MapType eMapType = MAPTYPE_UNKNOWN;

	time_t m_tNextUnpauseAllowed = 0;
};

extern CTournament g_Tournament;

extern ConVar tftrue_tournament_config;
extern ConVar tftrue_unpause_delay;
