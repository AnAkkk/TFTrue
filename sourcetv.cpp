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

#include "sourcetv.h"
#include "TFTrue.h"
#include "utils.h"
#include "tournament.h"
#include "editablecommands.h"

CSourceTV g_SourceTV;

ConVar tftrue_tv_delaymapchange("tftrue_tv_delaymapchange", "1", FCVAR_NOTIFY,"Delay the map change depending of tv_delay value when SourceTV is enabled in tournament mode.");
ConVar tftrue_tv_autorecord("tftrue_tv_autorecord", "1", FCVAR_NOTIFY,"Turn on/off auto STV recording when both teams are ready in tournament mode. It will stops when the win conditions are reached.", &CSourceTV::AutoRecord_Callback);
ConVar tftrue_tv_recordpath("tftrue_tv_demos_path", "");
ConVar tftrue_tv_prefix("tftrue_tv_prefix", "", FCVAR_NONE, "Prefix to add to the demo names with auto STV recording.", &CSourceTV::Prefix_Callback);
ConCommand *changelevel = nullptr;

CSourceTV::CSourceTV()
{
	memset(m_szTVRecord, 0, sizeof(m_szTVRecord));
}

bool CSourceTV::Init()
{
	changelevel = g_pCVar->FindCommand("changelevel");

	ConVarRef tv_enable("tv_enable");

	m_Enable_OldCallback = ((EditableConVar*)tv_enable.GetLinkedConVar())->m_fnChangeCallback;
	((EditableConVar*)tv_enable.GetLinkedConVar())->m_fnChangeCallback = &CSourceTV::Enable_Callback;

	ConVarRef tv_snapshotrate("tv_snapshotrate");
	tv_snapshotrate.SetValue("33");
	ConVarRef tv_maxrate("tv_maxrate");
	tv_maxrate.SetValue("0");

	ConCommand *changelevel = g_pCVar->FindCommand("changelevel");
	if(changelevel)
		m_DispatchChangeLevelRoute.RouteVirtualFunction(changelevel, &ConCommand::Dispatch, &CSourceTV::ChangeLevel_Callback, false);

	return changelevel;
}

void CSourceTV::OnUnload()
{
	ConVarRef tv_enable("tv_enable");
	((EditableConVar*)tv_enable.GetLinkedConVar())->m_fnChangeCallback = m_Enable_OldCallback;
}

void CSourceTV::OnTournamentStateUpdate(const char *szBlueTeamName, const char* szRedTeamName)
{
	static ConVarRef tv_enable("tv_enable");
	if(tv_enable.GetBool() && tftrue_tv_autorecord.GetBool())
	{
		char recordtime[20];
		time_t rawtime;
		struct tm * timeinfo;

		time ( &rawtime );
		timeinfo = localtime ( &rawtime );

		strftime(recordtime, 80, "%Y-%m-%d-%H-%M", timeinfo);

		char szFolderName[20] = {};

		if(strcmp(tftrue_tv_recordpath.GetString(),""))
		{
			V_snprintf(szFolderName, sizeof(szFolderName), "%.18s/", tftrue_tv_recordpath.GetString());
			filesystem->CreateDirHierarchy(szFolderName, "MOD");
		}

		if(strcmp(tftrue_tv_prefix.GetString(), ""))
			V_snprintf(m_szTVRecord, sizeof(m_szTVRecord), "tv_record \"%s%.50s-%s-%s_vs_%s-%s\"\n", szFolderName, tftrue_tv_prefix.GetString(), recordtime, szBlueTeamName, szRedTeamName, gpGlobals->mapname.ToCStr());
		else
			V_snprintf(m_szTVRecord, sizeof(m_szTVRecord), "tv_record \"%s%s-%s_vs_%s-%s\"\n", szFolderName, recordtime, szBlueTeamName, szRedTeamName, gpGlobals->mapname.ToCStr());

		engine->InsertServerCommand(m_szTVRecord);
		engine->ServerExecute();
	}
}

void CSourceTV::OnGameOver()
{
	StopTVRecord();
}

void CSourceTV::OnServerActivate()
{
	m_bDelayMapChange = false;
}

void CSourceTV::UpdateMapChangeDelay()
{
	if(gpGlobals->curtime > m_flNextMessage)
	{
		if(m_flTvDelay <= 0.0f)
		{
			ConCommand *pChangelevel = g_pCVar->FindCommand("changelevel");
			char szCommand[200];
			V_snprintf(szCommand, sizeof(szCommand), "changelevel %s\n", g_Plugin.GetNextMap());
			CCommand args;
			args.Tokenize(szCommand);
			g_Plugin.ForwardCommand(pChangelevel, args);

			m_bDelayMapChange = false;
		}
		else
		{
			Msg("[TFTrue] Changing map to %s in %.f seconds.\n", g_Plugin.GetNextMap(), m_flTvDelay);
			AllMessage("\003[TFTrue] Changing map to %s in %.f seconds.\n", g_Plugin.GetNextMap(), m_flTvDelay);

			if(m_flTvDelay < 30.0f)
			{
				m_flNextMessage = gpGlobals->curtime+m_flTvDelay;
				m_flTvDelay = 0.0f;
			}
			else
			{
				m_flNextMessage = gpGlobals->curtime+30.0f;
				m_flTvDelay -= 30;
			}
		}
	}
}

void CSourceTV::StopTVRecord()
{
	static ConVarRef tv_enable("tv_enable");
	static ConVarRef mp_tournament("mp_tournament");
	static ConVarRef tf_gamemode_mvm("tf_gamemode_mvm");

	if(tv_enable.GetBool() && tftrue_tv_autorecord.GetBool() && mp_tournament.GetBool()
			&& !tf_gamemode_mvm.GetBool())
	{
		engine->InsertServerCommand("tv_stoprecord\n");
		engine->ServerExecute();
	}
}

void CSourceTV::AutoRecord_Callback( IConVar *var, const char *pOldValue, float flOldValue )
{
	static ConVarRef tv_enable("tv_enable");
	if(!flOldValue && g_Tournament.BothTeamsReady() && tv_enable.GetBool())
	{
		engine->InsertServerCommand(g_SourceTV.m_szTVRecord);
		engine->ServerExecute();
	}
}

void CSourceTV::Enable_Callback( IConVar *var, const char *pOldValue, float flOldValue )
{
	g_SourceTV.m_Enable_OldCallback(var, pOldValue, flOldValue);

	ConVar* v = (ConVar*)var;
	if(v->GetBool() && !flOldValue)
	{
		AllMessage("\003[TFTrue] Source TV enabled! Changing map...\n");
		g_Plugin.ForceChangeMap(CTFTrue::FORCE_RELOADMAP, gpGlobals->curtime+3.0f);
	}
	else if(!v->GetBool() && flOldValue)
	{
		AllMessage("\003[TFTrue] Source TV disabled!\n");
		engine->InsertServerCommand("tv_stop\n");
		engine->ServerExecute();
	}
}

void CSourceTV::Prefix_Callback( IConVar *var, const char *pOldValue, float flOldValue )
{
	ConVar* v = (ConVar*)var;

	char szPrefix[50];

	V_strncpy(szPrefix, v->GetString(), sizeof(szPrefix));

	for(int i = 0; i < sizeof(szPrefix)/sizeof(char); i++)
	{
		if(!szPrefix[i])
			break;

		if((szPrefix[i] >= '0' && szPrefix[i] <= '9')
				|| (szPrefix[i] >= 'A' && szPrefix[i] <= 'Z')
				|| (szPrefix[i] >= 'a' && szPrefix[i] <= 'z'))
			continue;
		else
			szPrefix[i] = '_';
	}
	if(strcmp(v->GetString(), szPrefix) != 0)
		v->SetValue(szPrefix);
}

void CSourceTV::ChangeLevel_Callback(ConCommand *pCmd, const CCommand &args)
{
	static ConVarRef tv_delay("tv_delay");
	static ConVarRef tv_enable("tv_enable");
	static ConVarRef mp_tournament("mp_tournament");
	static ConVarRef tf_gamemode_mvm("tf_gamemode_mvm");

	if(engine->IsMapValid(args.Arg(1)))
	{
		if(g_Plugin.GetForceChangeMap() == CTFTrue::FORCE_NONE)
		{
			if(tv_enable.GetBool() && tftrue_tv_delaymapchange.GetBool() && mp_tournament.GetBool() && tv_delay.GetBool() && !tf_gamemode_mvm.GetBool())
			{
				if(g_SourceTV.m_bDelayMapChange)
				{
					g_SourceTV.m_bDelayMapChange = false;
					g_Plugin.ForceChangeMap(CTFTrue::FORCE_NEWMAP, gpGlobals->curtime + 3.0f);
					g_Plugin.SetNextMap(args.Arg(1));

					Msg("[TFTrue] Map change forced. Changing map to %s.\n", args.Arg(1));
					AllMessage("\003[TFTrue] Map change forced. Changing map to %s.\n", args.Arg(1));
				}
				else
				{
					g_SourceTV.m_bDelayMapChange = true;
					g_SourceTV.m_flTvDelay = tv_delay.GetFloat() - 30.0f;
					g_SourceTV.m_flNextMessage = gpGlobals->curtime + 30.0f;
					g_Plugin.SetNextMap(args.Arg(1));

					Msg("[TFTrue] Changing map to %s in %.f seconds. Use changelevel again to skip the delay.\n",
						args.Arg(1), tv_delay.GetFloat());
					AllMessage("\003[TFTrue] Changing map to %s in %.f seconds. Use changelevel again to skip the delay.\n",
							   args.Arg(1), tv_delay.GetFloat());
				}
			}
			else
				g_Plugin.ForwardCommand(pCmd, args);
		}
		else
		{
			g_Plugin.ForceChangeMap(CTFTrue::FORCE_NONE);
			g_Plugin.ForwardCommand(pCmd, args);
		}
	}
}
