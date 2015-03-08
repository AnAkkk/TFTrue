#pragma once

#include "SDK.h"

class CSourceTV
{
public:
	CSourceTV();

	bool Init();
	void OnUnload();
	void OnGameOver();
	void OnTournamentStateUpdate(const char *szBlueTeamName, const char *szRedTeamName);
	void OnServerActivate();
	bool OnDispatchCommand(ConCommand *pCmd, const CCommand &args);

	void StopTVRecord();
	void UpdateMapChangeDelay();
	bool IsDelayingMapChange() { return m_bDelayMapChange; }

	static void AutoRecord_Callback( IConVar *var, const char *pOldValue, float flOldValue );
	static void Enable_Callback( IConVar *var, const char *pOldValue, float flOldValue );
	static void Prefix_Callback( IConVar *var, const char *pOldValue, float flOldValue );
	void ChangeLevel_Callback(ConCommand *pCmd, const CCommand &args);
private:
	char m_szTVRecord[255];
	FnChangeCallback_t m_Enable_OldCallback = nullptr;

	bool m_bDelayMapChange = false;
	float m_flTvDelay = 0.0f;
	float m_flNextMessage = 0.0f;
};

extern CSourceTV g_SourceTV;

extern ConVar tftrue_tv_delaymapchange;
extern ConVar tftrue_tv_autorecord;
extern ConVar tftrue_tv_recordpath;
extern ConVar tftrue_tv_prefix;
