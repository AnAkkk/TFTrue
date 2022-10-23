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

#include <thread>
#include <string>

#include "TFTrue.h"
#include "AutoUpdater.h"
#include "utils.h"
#include "items.h"
#include "ModuleScanner.h"
#include "stats.h"
#include "logs.h"
#include "bunnyhop.h"
#include "sourcetv.h"
#include "fov.h"
#include "tournament.h"
#include "editablecommands.h"

#ifdef DEBUG
#define NO_AUTOUPDATE
#endif

CTFTrue g_Plugin;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CTFTrue, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, g_Plugin )

ConVar tftrue_version("tftrue_version", "4.87", FCVAR_NOTIFY|FCVAR_CHEAT,
	"Version of the plugin.",
    &CTFTrue::Version_Callback);
ConVar tftrue_gamedesc("tftrue_gamedesc", "", FCVAR_NONE,
	"Set the description you want to show in the game description column of the server browser. Max 40 characters.",
    &CTFTrue::GameDesc_Callback);
ConVar tftrue_freezecam("tftrue_freezecam", "1", FCVAR_NOTIFY,
	"Activate/Deactivate the freeze cam.",
	true, 0, true, 1,
	&CTFTrue::Freezecam_Callback);

IVEngineServer *engine                  = nullptr;
IPlayerInfoManager *playerinfomanager   = nullptr;
IServerGameDLL *gamedll                 = nullptr;
IServerGameEnts *gameents               = nullptr;
CGlobalVars *gpGlobals                  = nullptr;
CGlobalEntityList *pEntList             = nullptr;
CBaseEntityList *g_pEntityList          = nullptr;
IFileSystem *filesystem                 = nullptr;
IGameEventManager2* gameeventmanager    = nullptr;
IServerPluginHelpers* helpers           = nullptr;
IServer* g_pServer                      = nullptr;
IGameMovement* gamemovement             = nullptr;
CGameMovement* g_pGameMovement          = nullptr;
IEngineReplay* g_pEngineReplay          = nullptr;
IServerGameClients* g_pGameClients      = nullptr;
IEngineTrace* g_pEngineTrace            = nullptr;
IServerTools* g_pServerTools            = nullptr;

CSteamGameServerAPIContext steam;

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is loaded, load the interface we need from the engine
//---------------------------------------------------------------------------------
bool CTFTrue::Load( CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory )
{
	++m_iLoadCount;

	if (m_iLoadCount <= 1)
	{
		engine                = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER, NULL);
		playerinfomanager     = (IPlayerInfoManager *)gameServerFactory(INTERFACEVERSION_PLAYERINFOMANAGER, NULL);
		g_pCVar               = (ICvar*)interfaceFactory( CVAR_INTERFACE_VERSION, NULL );
		gamedll               = (IServerGameDLL*)gameServerFactory(INTERFACEVERSION_SERVERGAMEDLL, NULL);
		gameents              = (IServerGameEnts*)gameServerFactory(INTERFACEVERSION_SERVERGAMEENTS, NULL);
		filesystem            = (IFileSystem*)interfaceFactory(FILESYSTEM_INTERFACE_VERSION, NULL);
		gameeventmanager      = (IGameEventManager2*)interfaceFactory( INTERFACEVERSION_GAMEEVENTSMANAGER2, NULL );
		helpers               = (IServerPluginHelpers*)interfaceFactory( INTERFACEVERSION_ISERVERPLUGINHELPERS, NULL);
		gamemovement          = (IGameMovement*)gameServerFactory( INTERFACENAME_GAMEMOVEMENT, NULL);
		g_pEngineReplay       = (IEngineReplay*)interfaceFactory(ENGINE_REPLAY_INTERFACE_VERSION, NULL);
		g_pGameClients        = (IServerGameClients*)gameServerFactory(INTERFACEVERSION_SERVERGAMECLIENTS, NULL);
		g_pEngineTrace        = (IEngineTrace*)interfaceFactory(INTERFACEVERSION_ENGINETRACE_SERVER, NULL);
		g_pServerTools        = (IServerTools*)gameServerFactory(VSERVERTOOLS_INTERFACE_VERSION, NULL);

		if (g_pEngineReplay)
		{
			g_pServer = g_pEngineReplay->GetGameServer();
		}

		if
		(
			   !engine
			|| !playerinfomanager
			|| !g_pCVar
			|| !gamedll
			|| !gameents
			|| !filesystem
			|| !helpers
			|| !gamemovement
			|| !gameeventmanager
			|| !g_pEngineReplay
			|| !g_pGameClients
			|| !g_pEngineTrace
			|| !g_pServerTools
			|| !g_pServer
		)
		{
			Warning("[TFTrue] Can't load needed interfaces!\n");
			return false;
		}

		if (playerinfomanager)
		{
			gpGlobals = playerinfomanager->GetGlobalVars();
		}

		g_pGameMovement = (CGameMovement*)gamemovement;

		steam.Init();

		UpdateGameDesc();

		ConVar_Register();
		MathLib_Init();

		CModuleScanner ServerModule((void*)gameServerFactory);
		CModuleScanner EngineModule((void*)interfaceFactory);

		g_AutoUpdater.Init();
		if(!g_Stats.Init(ServerModule))
		{
			return false;
		}
		if(!g_Logs.Init(EngineModule, ServerModule))
		{
			return false;
		}
		if(!g_Items.Init(ServerModule))
		{
			return false;
		}
		if(!g_BunnyHop.Init(ServerModule))
		{
			return false;
		}
		if(!g_SourceTV.Init())
		{
			return false;
		}
		if(!g_Tournament.Init(EngineModule, ServerModule))
		{
			return false;
		}

		g_FOV.OnLoad();

		m_GetGameDescriptionRoute.RouteVirtualFunction(gamedll, &IServerGameDLL::GetGameDescription, &CTFTrue::GetGameDescription);
		m_ChangeLevelRoute.RouteVirtualFunction(engine, &IVEngineServer::ChangeLevel, &CTFTrue::ChangeLevel);
		m_GameServerSteamAPIActivatedRoute.RouteVirtualFunction(gamedll, &IServerGameDLL::GameServerSteamAPIActivated, &CTFTrue::GameServerSteamAPIActivated);

		pEntList      = g_pServerTools->GetEntityList();
		g_pEntityList = pEntList;

		// Fix for Freezecam reenabling itself if sv_cheats is toggled
		ConVarRef spec_freeze_time("spec_freeze_time");
		((EditableConVar*)spec_freeze_time.GetLinkedConVar())->m_nFlags &= ~FCVAR_CHEAT;
		ConVarRef spec_freeze_traveltime("spec_freeze_traveltime");
		((EditableConVar*)spec_freeze_traveltime.GetLinkedConVar())->m_nFlags &= ~FCVAR_CHEAT;

		ConCommand *say = g_pCVar->FindCommand("say");
		if (say)
		{
			m_DispatchSayRoute.RouteVirtualFunction(say, &ConCommand::Dispatch, &CTFTrue::Say_Callback, false);
		}

		EditableConCommand *plugin_load = (EditableConCommand*)g_pCVar->FindCommand("plugin_load");

		// I *think* this is removing the "can't load plugin while running map" restriction with plugin_load but idk
		if (plugin_load)
		{
		#ifndef _LINUX
			PatchAddress((void*)plugin_load->m_fnCommandCallback, 0x13, 1, (unsigned char*)"\xEB");
		#else
			PatchAddress((void*)plugin_load->m_fnCommandCallback, 0x1B, 6, (unsigned char*)"\x90\x90\x90\x90\x90\x90");
		#endif
		}
		// Not working under Windows SRCDS but working under Linux, useless though as people can use pause
		ConCommand *setpause = g_pCVar->FindCommand("setpause");
		ConCommand *unpause  = g_pCVar->FindCommand("unpause");
		if(setpause)
		{
			g_pCVar->UnregisterConCommand(setpause);
		}
		if(unpause)
		{
			g_pCVar->UnregisterConCommand(unpause);
		}

		// If we are late loading, this is required
		m_pEdictList = engine->PEntityOfEntIndex(0);

		// Fix for A2S_RULES after updating the plugin
		tftrue_version.SetValue(tftrue_version.GetString());

		return true;
	}
	else
	{
		Msg("[TFTrue] The plugin is already loaded.\n");
		return false;
	}
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is unloaded (turned off)
//---------------------------------------------------------------------------------
void CTFTrue::Unload( void )
{
	if(m_iLoadCount <= 1 && !m_bReloadedNeeded)
	{
		tftrue_freezecam.SetValue("1");
		tftrue_bunnyhop.SetValue("0");

		ConVar_Unregister();

		g_BunnyHop.OnUnload();
		g_FOV.OnUnload();
		g_Logs.OnUnload();
		g_Stats.OnUnload();
		g_Items.OnUnload();
		g_SourceTV.OnUnload();
		g_Tournament.OnUnload();

		#ifndef _LINUX
		// Fix plugin unloading
		// Blame Microsoft
		// connect.microsoft.com/VisualStudio/feedback/details/781665/
		MEMORY_BASIC_INFORMATION mbi;
		static int dummy;
		VirtualQuery( &dummy, &mbi, sizeof(mbi) );

		// Get the reference count on every iteration
		// because calling FreeLibrary can decrement it twice
		while(GetModuleLoadCount((HMODULE)mbi.AllocationBase) > 1)
			FreeLibrary((HMODULE)mbi.AllocationBase);
		#endif
	}

	--m_iLoadCount;
}


//---------------------------------------------------------------------------------
// Purpose: the name of this plugin, returned in "plugin_print" command
//---------------------------------------------------------------------------------
char PluginDesc[50];
const char *CTFTrue::GetPluginDescription( void )
{
	sprintf(PluginDesc, "TFTrue v%s, AnAkkk", tftrue_version.GetString());

	return PluginDesc;
}


void CTFTrue::SetCommandClient( int index )
{
	m_iClientCommandIndex = index;
}

void CTFTrue::GameFrame( bool simulating )
{
	if(m_bForceReloadMap)
	{
		if((gpGlobals->curtime > m_flNextReloadMap) || !simulating)
		{
			char szCommand[200];
			CCommand args;
			ConCommand *pChangelevel = g_pCVar->FindCommand("changelevel");

			V_snprintf(szCommand, sizeof(szCommand), "changelevel %s\n", gpGlobals->mapname.ToCStr());
			args.Tokenize(szCommand);
			ForwardCommand(pChangelevel, args);

			m_bForceReloadMap = false;
		}
	}
	#ifndef NO_AUTOUPDATE
	g_AutoUpdater.OnGameFrame();
	#endif
	g_Logs.OnGameFrame();
}

PLUGIN_RESULT CTFTrue::ClientCommand( edict_t *pEntity, const CCommand &args )
{
	const char *cmd = args.Arg(0);

	if( Q_stricmp(cmd, "tftrue") == 0 )
	{
		PrintTFTrueInfos(pEntity);
		return PLUGIN_STOP;
	}
	else if(!stricmp(cmd, "jointeam"))
	{
		g_Stats.OnJoinTeam(pEntity);
	}
	else if(!stricmp(cmd, "joinclass"))
	{
		g_BunnyHop.OnJoinClass(pEntity);
	}
	return PLUGIN_CONTINUE;
}


void CTFTrue::ServerActivate( edict_t *pEdictList, int edictCount, int clientMax )
{
	m_pEdictList = pEdictList;
	m_flNextReloadMap = 0.0f;
	m_bForceReloadMap = false;

	m_pGameRulesData = nullptr;
	CBaseEntity *pGameRulesProxy = g_pServerTools->FindEntityByClassname(nullptr, "tf_gamerules");
	if(pGameRulesProxy)
	{
		SendTable *pTable = pGameRulesProxy->GetServerClass()->m_pTable;
		if(pTable)
		{
			SendProp *pProp = g_SendProp.GetSendProp(pTable, "tf_gamerules_data");
			if(pProp)
			{
				auto proxyFn = pProp->GetDataTableProxyFn();
				if(proxyFn)
				{
					CSendProxyRecipients recipients;
					m_pGameRulesData = proxyFn(nullptr, nullptr, nullptr, &recipients, 0);
				}
			}
		}
	}

	g_Tournament.OnServerActivate();
	g_Logs.OnServerActivate();
	g_Stats.OnServerActivate();
}

// This function will create an accurate game name for us
void CTFTrue::UpdateGameDesc()
{
	V_snprintf(m_szGameDesc, sizeof(m_szGameDesc), "TFTrue %s ", tftrue_gamedesc.GetString());

	if(steam.SteamGameServer())
	{
		steam.SteamGameServer()->SetGameDescription(m_szGameDesc);
	}
}

const char* CTFTrue::GetGameDescription(IServerGameDLL *gamedll EDX2)
{
	g_Plugin.UpdateGameDesc();

	return g_Plugin.m_szGameDesc;
}

void CTFTrue::ClientDisconnect(edict_t *pEntity)
{
	g_Logs.OnDisconnect(pEntity);

	// Don't call stuff below on map change
	if(!engine->PEntityOfEntIndex(0))
	{
		return;
	}

	// Don't call stuff below for bots
	if(!engine->GetPlayerNetInfo(IndexOfEdict(pEntity)))
	{
		return;
	}

	g_Stats.OnDisconnect(pEntity);
	g_FOV.OnPlayerDisconnect(pEntity);
	g_BunnyHop.OnPlayerDisconnect(pEntity);
}

void CTFTrue::ClientSettingsChanged( edict_t *pEdict )
{
	g_FOV.OnClientSettingsChanged(pEdict);
}

void CTFTrue::ForceReloadMap(float flTime)
{
	m_bForceReloadMap = true;
	m_flNextReloadMap = flTime;
}

void CTFTrue::Version_Callback( IConVar *var, const char *pOldValue, float flOldValue )
{
	ConVar* v = (ConVar*)var;
	if(strcmp(v->GetString(), v->GetDefault()))
	{
		v->Revert();
	}
}

void CTFTrue::GameDesc_Callback( IConVar *var, const char *pOldValue, float flOldValue )
{
	g_Plugin.UpdateGameDesc();
}

void CTFTrue::GameServerSteamAPIActivated(IServerGameDLL *gamedll EDX2)
{
	typedef void (__thiscall *GameServerSteamAPIActivated_t)(IServerGameDLL *gamedll);
	g_Plugin.m_GameServerSteamAPIActivatedRoute.CallOriginalFunction<GameServerSteamAPIActivated_t>()(gamedll);

	if(steam.Init())
	{
	#ifndef NO_AUTOUPDATE
		g_AutoUpdater.CheckUpdate();
	#endif
	}
}

void CTFTrue::Say_Callback(ConCommand *pCmd, EDX const CCommand &args)
{
	if(g_Plugin.GetCommandIndex() == -1)
	{
		g_Plugin.ForwardCommand(pCmd, args);
		return;
	}

	if(g_pServer->IsPaused())
	{
		CBasePlayer *pPlayer = (CBasePlayer*)CBaseEntity::Instance(g_Plugin.GetCommandIndex()+1);
		// Set last talk time to 0.0f
		*(float*)(g_EntityProps.GetSendProp<char>(pPlayer, "m_hLastWeapon")-4) = 0.0f;
	}

	const char *Text = "";
	if(args.ArgC() == 2)
	{
		Text = args.Arg(1);
	}
	else
	{
		Text = args.ArgS();
	}

	if(strcmp(Text, "!tftrue") == 0)
	{
		Message(g_Plugin.GetCommandIndex()+1, "\003TFTrue Version %s", tftrue_version.GetString() );
		Message(g_Plugin.GetCommandIndex()+1, "\003Freeze Cam: \005%s",(tftrue_freezecam.GetBool() == true ) ? "On":"Off");

		char szLine[255];
		sprintf(szLine, "\003Items: Hats: \005%s \003Misc: \005%s \003Action: \005%s",
				(tftrue_no_hats.GetBool() == true ) ? "Off":"On",
				(tftrue_no_misc.GetBool() == true ) ? "Off":"On",
				(tftrue_no_action.GetBool() == true ) ? "Off":"On");

		int iItemLineLength = strlen(szLine);

		if (tftrue_whitelist_id.GetInt() != -1)
		{
			if(!strcmp(g_Items.GetWhitelistName(), "custom"))
			{
				sprintf(szLine+iItemLineLength, " \003Custom Whitelist ID: \005%d\n", tftrue_whitelist_id.GetInt());
			}
			else
			{
				sprintf(szLine+iItemLineLength, " \003Whitelist:\005%s", g_Items.GetWhitelistName());
			}
		}
		else
			sprintf(szLine+iItemLineLength, " \003Whitelist: \005None");

		Message(g_Plugin.GetCommandIndex()+1, "%s", szLine);

		Message(g_Plugin.GetCommandIndex()+1, "\003Maximum FoV allowed: \005%d",(tftrue_maxfov.GetInt()));
		Message(g_Plugin.GetCommandIndex()+1, "\003Bunny Hop: \005%s",(tftrue_bunnyhop.GetBool() == true) ? "On":"Off");
		Message(g_Plugin.GetCommandIndex()+1, "\003Restore stats: \005%s",(tftrue_restorestats.GetBool() == true) ? "On":"Off");

		static ConVarRef mp_tournament("mp_tournament");
		static ConVarRef tf_gamemode_mvm("tf_gamemode_mvm");

		if (mp_tournament.GetBool() && !tf_gamemode_mvm.GetBool())
		{
			Message(g_Plugin.GetCommandIndex()+1, "\003STV Autorecord: \005%s",(tftrue_tv_autorecord.GetBool() == true ) ? "On":"Off");

			switch(tftrue_tournament_config.GetInt())
			{
			case CTournament::CONFIG_NONE:
				Message(g_Plugin.GetCommandIndex()+1, "\003Tournament config: \005None");
				break;
			case CTournament::CONFIG_ETF2L6v6:
				Message(g_Plugin.GetCommandIndex()+1, "\003Tournament config: \005ETF2L 6v6");
				break;
			case CTournament::CONFIG_ETF2L9v9:
				Message(g_Plugin.GetCommandIndex()+1, "\003Tournament config: \005ETF2L 9v9");
				break;
			}
		}
	}
	else if(strstr(Text, "!fov") == Text)
	{
		unsigned int fov = 0;
		sscanf(Text, "!fov %u", &fov);

		g_FOV.OnFOVCommand(fov);
	}
	else if(strstr(Text, "!speedmeter") == Text)
	{
		if(strcmp(Text, "!speedmeter on") == 0)
		{
			g_BunnyHop.SetSpeedMeter(g_Plugin.GetCommandIndex(), true);
		}
		else if(strcmp(Text,"!speedmeter off") == 0)
		{
			g_BunnyHop.SetSpeedMeter(g_Plugin.GetCommandIndex(), false);
		}
		else
		{
			Message(g_Plugin.GetCommandIndex()+1,"\003Usage : !speedmeter [on/off]");
		}
	}
	else if(!strcmp(Text, "!log"))
		g_Logs.OnLogCommand();
	else
		g_Plugin.ForwardCommand(pCmd, args);
}

void CTFTrue::ChangeLevel(IVEngineServer *pServer, EDX const char *s1, const char *s2)
{
	if(!s1)
		return;

	char szCmd[256];
	if(!s2)
	{
		V_snprintf(szCmd, sizeof(szCmd), "changelevel %s\n", s1);
	}
	else
	{
		V_snprintf(szCmd, sizeof(szCmd), "changelevel %s %s\n", s1, s2);
	}

	engine->ServerCommand(szCmd);
}

void CTFTrue::ForwardCommand(ConCommand *pCmd, const CCommand &args)
{
	typedef void (__thiscall *Dispatch_t)(ConCommand *pCmd, const CCommand &args);
	m_DispatchSayRoute.CallOriginalFunction<Dispatch_t>()(pCmd, args);
}

void CTFTrue::Freezecam_Callback( IConVar *var, const char *pOldValue, float flOldValue )
{
	ConVar* v = (ConVar*)var;
	static ConVarRef spec_freeze_time("spec_freeze_time");
	static ConVarRef spec_freeze_traveltime("spec_freeze_traveltime");

	if(v->GetBool())
	{
		((ConVar*)spec_freeze_time.GetLinkedConVar())->Revert();
		((ConVar*)spec_freeze_traveltime.GetLinkedConVar())->Revert();
	}
	else
	{
		spec_freeze_time.SetValue("0");
		spec_freeze_traveltime.SetValue("0");
	}
}
