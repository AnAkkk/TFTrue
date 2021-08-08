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
#include "sourcetv.h"
#include "tournament.h"
#include "editablecommands.h"


CTournament g_Tournament;

int *g_sv_pure_mode = nullptr;
int *cmd_source = nullptr;
int *cmd_clientslot = nullptr;
EditableConCommand *sv_pure = nullptr;
ConCommand *status = nullptr;
ConCommand *mp_tournament_restart = nullptr;
ConCommand *pause_ = nullptr;

ConVar tftrue_unpause_delay("tftrue_unpause_delay", "2", FCVAR_NOTIFY,
							"Set the delay before someone can unpause the game after it has been paused.");

bool CTournament::Init(const CModuleScanner& EngineModule, const CModuleScanner& ServerModule)
{
	gameeventmanager->AddListener(this, "teamplay_game_over", true);
	gameeventmanager->AddListener(this, "tf_game_over", true);
	gameeventmanager->AddListener(this, "teamplay_round_win", true);
	gameeventmanager->AddListener(this, "teamplay_round_stalemate", true);

	mp_tournament_restart = g_pCVar->FindCommand("mp_tournament_restart");
	sv_pure = (EditableConCommand*)g_pCVar->FindCommand("sv_pure");
	status = g_pCVar->FindCommand("status");
	pause_ = g_pCVar->FindCommand("pause");

	ConVarRef mp_tournament("mp_tournament");
	ConVarRef sv_pausable("sv_pausable");

	if(!(mp_tournament.IsValid() && sv_pausable.IsValid() && mp_tournament_restart && sv_pure && status && pause_))
	{
		Warning("[TFTrue] Can't find tournament cvars\n");
		return false;
	}

	ConCommand *mp_tournament_restart = g_pCVar->FindCommand("mp_tournament_restart");
	if(mp_tournament_restart)
		m_DispatchTournamentRestartRoute.RouteVirtualFunction(mp_tournament_restart, &ConCommand::Dispatch, &CTournament::Tournament_Restart_Callback, false);

	EditableConCommand *sv_pure = (EditableConCommand*)g_pCVar->FindCommand("sv_pure");
	if(sv_pure)
		m_DispatchPureRoute.RouteVirtualFunction(sv_pure, &ConCommand::Dispatch, &CTournament::Pure_Callback, false);

	EditableConCommand *status = (EditableConCommand*)g_pCVar->FindCommand("status");
	if(status)
		m_DispatchStatusRoute.RouteVirtualFunction(status, &ConCommand::Dispatch, &CTournament::Status_Callback, false);

	ConCommand *pause = g_pCVar->FindCommand("pause");
	if(pause)
		m_DispatchPauseRoute.RouteVirtualFunction(pause, &ConCommand::Dispatch, &CTournament::Pause_Callback, false);

#ifdef _LINUX
	void *pStartCompetitiveMatch = ServerModule.FindSymbol("_ZN12CTFGameRules21StartCompetitiveMatchEv");
#else
	void *pStartCompetitiveMatch = ServerModule.FindSignature("\x56\x8B\xF1\x6A\x00\xC7\x86", "xxxxxxx");
#endif
	if(pStartCompetitiveMatch)
		m_StartCompetitiveMatchRoute.RouteFunction(pStartCompetitiveMatch, (void*)&CTournament::StartCompetitiveMatch);
	else
		Warning("[TFTrue] Failed to find StartCompetitiveMatch\n");

#ifdef _LINUX
	cmd_source = (int*)EngineModule.FindSymbol("cmd_source");
	cmd_clientslot = (int*)EngineModule.FindSymbol("cmd_clientslot");
	void *CanPlayerChooseClass = ServerModule.FindSymbol("_ZN12CTFGameRules20CanPlayerChooseClassEP11CBasePlayeri");
	g_sv_pure_mode = (int*)EngineModule.FindSymbol("_ZL14g_sv_pure_mode");
#else
	cmd_source = *(int**)((unsigned char*)status->m_fnCommandCallback + 0xB);
	cmd_clientslot = *(int**)((unsigned char*)status->m_fnCommandCallback + 0x38);
	void *CanPlayerChooseClass = ServerModule.FindSignature(
				(unsigned char*)"\x55\x8B\xEC\x83\xEC\x08\xFF\x75\x0C\xE8\x00","xxxxxxxxxx?");
	g_sv_pure_mode = *(int**)((unsigned char*)sv_pure->m_fnCommandCallback + 0x58);
#endif

	if(!cmd_source || !cmd_clientslot || !CanPlayerChooseClass || !g_sv_pure_mode)
	{
		Warning("[TFTrue] Can't find tournament pointers\n");
		return false;
	}

	((ConVar*)mp_tournament.GetLinkedConVar())->InstallChangeCallback(&CTournament::Tournament_Callback);

#ifndef _LINUX
	// CanPlayerChooseClass calls another function that calls IsInTournamentMode, so we need the address of that other function
	unsigned long CanPlayerChooseClass_TournamentCall 	= (unsigned long)((unsigned char*)CanPlayerChooseClass + 0xA);
	unsigned long CanPlayerChooseClass_TournamentOffset = *(unsigned long*)(CanPlayerChooseClass_TournamentCall);
	unsigned long CanPlayerChooseClass_Tournament 		= CanPlayerChooseClass_TournamentOffset + CanPlayerChooseClass_TournamentCall + 4;

	PatchAddress((void*)CanPlayerChooseClass_Tournament, 0xD, 1, (unsigned char*)"\xEB");
#else
	/*
	// CanPlayerChooseClass calls another function that calls IsInTournamentMode, so we need the address of that other function
	// FIXME
	// STILL TODO -steph
	// + 16
	unsigned long CanPlayerChooseClass_TournamentCall		= (unsigned long)((unsigned char*)CanPlayerChooseClass + 0x1C);
	unsigned long CanPlayerChooseClass_TournamentOffset		= *(unsigned long*)(CanPlayerChooseClass_TournamentCall);
	unsigned long CanPlayerChooseClass_Tournament			= 11 + CanPlayerChooseClass_TournamentCall + 4;
	//unsigned long CanPlayerChooseClass_Tournament = ServerModule.FindSymbol("_ZN24CTeamplayRoundBasedRules18IsInTournamentModeEv");

	PatchAddress((void*)CanPlayerChooseClass_Tournament, 0x1C, 2, (unsigned char*)"\x90\x90");
	*/
#endif

	return true;
}

void CTournament::OnUnload()
{
	gameeventmanager->RemoveListener(this);

	ConVarRef mp_tournament("mp_tournament");

	((ConVar*)mp_tournament.GetLinkedConVar())->InstallChangeCallback(nullptr);
}

void CTournament::OnServerActivate()
{
	m_bTournamentStarted = false;
}

void CTournament::StartCompetitiveMatch(void *pGameRules EDX2)
{
	typedef void (__thiscall *StartCompetitiveMatch_t)(void *pGameRules);
	g_Tournament.m_StartCompetitiveMatchRoute.CallOriginalFunction<StartCompetitiveMatch_t>()(pGameRules);

	static ConVarRef mp_tournament_blueteamname("mp_tournament_blueteamname");
	static ConVarRef mp_tournament_redteamname("mp_tournament_redteamname");

	char szBlueTeamName[10];
	char szRedTeamName[10];

	V_strncpy(szBlueTeamName, mp_tournament_blueteamname.GetString(), sizeof(szBlueTeamName));
	V_strncpy(szRedTeamName, mp_tournament_redteamname.GetString(), sizeof(szRedTeamName));

	for(int i = 0; i < sizeof(szBlueTeamName)/sizeof(char); i++)
	{
		if(!szBlueTeamName[i])
			break;

		if((szBlueTeamName[i] >= '0' && szBlueTeamName[i] <= '9')
				|| (szBlueTeamName[i] >= 'A' && szBlueTeamName[i] <= 'Z')
				|| (szBlueTeamName[i] >= 'a' && szBlueTeamName[i] <= 'z'))
			continue;
		else
			szBlueTeamName[i] = '_';
	}

	for(int i = 0; i < sizeof(szRedTeamName)/sizeof(char); i++)
	{
		if(!szRedTeamName[i])
			break;

		if((szRedTeamName[i] >= '0' && szRedTeamName[i] <= '9')
				|| (szRedTeamName[i] >= 'A' && szRedTeamName[i] <= 'Z')
				|| (szRedTeamName[i] >= 'a' && szRedTeamName[i] <= 'z'))
			continue;
		else
			szRedTeamName[i] = '_';
	}

	g_SourceTV.OnTournamentStarted(szBlueTeamName, szRedTeamName);

	char szMsg[100];
	V_snprintf(szMsg, sizeof(szMsg), "Tournament mode started\nBlue Team: %s\nRed Team: %s\n", szBlueTeamName, szRedTeamName);
	engine->LogPrint(szMsg);

	static ConVarRef sv_cheats("sv_cheats");
	if(sv_cheats.GetBool())
		AllMessage("\003[TFTrue] WARNING: Cheats are enabled !\n");

	if(*g_sv_pure_mode != 2)
		AllMessage("\003[TFTrue] WARNING: The server is not correctly set up: sv_pure is not enabled!\n");

	if(g_Tournament.m_iConfigDownloadFailed)
		AllMessage("\003[TFTrue] WARNING: The download of %d tournament config files failed! "
				   "The server might not be setup correctly.\n", g_Tournament.m_iConfigDownloadFailed);

	g_Logs.OnTournamentStarted();

	g_Tournament.m_bTournamentStarted = true;
}

void CTournament::FireGameEvent(IGameEvent *pEvent)
{
	if(!strcmp(pEvent->GetName(),"teamplay_game_over") || !strcmp(pEvent->GetName(), "tf_game_over"))
	{
		if(m_bTournamentStarted)
		{
			m_bTournamentStarted = false;

			g_SourceTV.OnGameOver();
			g_Logs.OnGameOver();
		}
	}
	else if(!strcmp(pEvent->GetName(), "teamplay_round_win") || !strcmp(pEvent->GetName(), "teamplay_round_stalemate"))
	{
		if(m_bTournamentStarted)
			g_Logs.OnRoundWin();
	}
}

void CTournament::Tournament_Callback( IConVar *var, const char *pOldValue, float flOldValue )
{
	static ConVarRef tf_gamemode_mvm("tf_gamemode_mvm");

	ConVar* v = (ConVar*)var;
	if (v->GetBool() && !flOldValue)
	{
		if(*g_sv_pure_mode == 2 && !tf_gamemode_mvm.GetBool())
			sv_pure->m_nFlags |= FCVAR_DEVELOPMENTONLY;

	}
	else if(!v->GetBool() && flOldValue)
		sv_pure->m_nFlags &= ~FCVAR_DEVELOPMENTONLY;
}

void CTournament::Tournament_Restart_Callback(ConCommand *pCmd, EDX const CCommand &args)
{
	g_Plugin.ForwardCommand(pCmd, args);

	if(g_Plugin.GetCommandIndex() == -1)
	{
		g_Tournament.m_bTournamentStarted = false;
		g_SourceTV.StopTVRecord();
		g_Logs.ResetLastLogID();

		// TODO: create a new log if we're auto uploading?
	}
}



void CTournament::Pure_Callback(ConCommand *pCmd, EDX const CCommand &args)
{
	static ConVarRef mp_tournament("mp_tournament");
	static ConVarRef tf_gamemode_mvm("tf_gamemode_mvm");

	int iOldPureValue = *g_sv_pure_mode;

	g_Plugin.ForwardCommand(pCmd, args);

	int iNewPureValue = *g_sv_pure_mode;

	if(iNewPureValue == 2 && iOldPureValue != 2)
	{
		if(mp_tournament.GetBool() && !tf_gamemode_mvm.GetBool())
			sv_pure->m_nFlags |= FCVAR_DEVELOPMENTONLY;

		AllMessage("\003[TFTrue] sv_pure changed to 2. Changing map...\n");
		g_Plugin.ForceReloadMap(gpGlobals->curtime+3.0f);
	}
	else if(iNewPureValue == 1 && iOldPureValue != 1)
	{
		AllMessage("\003[TFTrue] sv_pure changed to 1. Changing map...\n");
		g_Plugin.ForceReloadMap(gpGlobals->curtime+3.0f);
	}
	else if(iNewPureValue == 0 && iOldPureValue != 0)
	{
		AllMessage("\003[TFTrue] sv_pure changed to 0. Changing map...\n");
		g_Plugin.ForceReloadMap(gpGlobals->curtime+3.0f);
	}
	else if(iNewPureValue == -1 && iOldPureValue != -1)
	{
		AllMessage("\003[TFTrue] sv_pure changed to -1. Changing map...\n");
		g_Plugin.ForceReloadMap(gpGlobals->curtime+3.0f);
	}
}

void CTournament::Status_Callback(ConCommand *pCmd, EDX const CCommand &args)
{
	g_Plugin.ForwardCommand(pCmd, args);

	static ConVarRef mp_tournament("mp_tournament");
	static ConVarRef tf_gamemode_mvm("tf_gamemode_mvm");

	if(cmd_source && *cmd_source == 1)
	{
		static ConCommand* plugin_print = g_pCVar->FindCommand("plugin_print");
		plugin_print->Dispatch(CCommand());
	}
	else if(cmd_clientslot && *cmd_clientslot != -1 && mp_tournament.GetBool() && !tf_gamemode_mvm.GetBool())
	{
		edict_t* pEntity = EdictFromIndex(*cmd_clientslot+1);
		if(helpers)
		{
			engine->ClientPrintf(pEntity, "Loaded plugins:\n");
			engine->ClientPrintf(pEntity, "---------------------\n");
			CUtlVector<CPlugin *> *m_Plugins = (CUtlVector<CPlugin *>*)((char*)helpers+4);
			for ( int i = 0; i < m_Plugins->Count(); i++ )
			{
				char szPluginName[255];
				V_snprintf(szPluginName, sizeof(szPluginName), "%s\n", m_Plugins->Element(i)->m_szName);

				static ConVarRef db_silent("db_silent");

				if(strstr(szPluginName, "DBlocker") && db_silent.GetBool())
					continue;
				if(strstr(szPluginName, "AnAkIn's Anti-Cheat"))
					continue;

				engine->ClientPrintf(pEntity, szPluginName);
			}
			engine->ClientPrintf(pEntity, "---------------------\n");
		}

		PrintTFTrueInfos(pEntity);
	}
}

void CTournament::Pause_Callback(ConCommand *pCmd, EDX const CCommand &args)
{
	static ConVarRef sv_pausable("sv_pausable");

	if(sv_pausable.GetBool() && *cmd_source == 0)
	{
		if(g_pServer->IsPaused()) // Server is paused
		{
			if(time(NULL) >= g_Tournament.m_tNextUnpauseAllowed)
			{
				IClient *pClient = g_pServer->GetClient(*cmd_clientslot);
				AllMessage(*cmd_clientslot+1, "\x05[TFTrue] The game was unpaused by \x03%s\x05.\n", pClient->GetClientName());

				g_Plugin.ForwardCommand(pCmd, args);
			}
			else
				Message(*cmd_clientslot+1, "\x07%sPlease wait %ld seconds before unpausing!\n", "FF0000", g_Tournament.m_tNextUnpauseAllowed - time(NULL));
		}
		else // Server is not paused
		{
			g_Plugin.ForwardCommand(pCmd, args);
			g_Tournament.m_tNextUnpauseAllowed = time(NULL) + tftrue_unpause_delay.GetInt();
			IClient *pClient = g_pServer->GetClient(*cmd_clientslot);
			AllMessage(*cmd_clientslot+1, "\x05[TFTrue] The game was paused by \x03%s\x05.\n", pClient->GetClientName());
		}
	}
}


// Download a config file from a persistent connection
// The socket needs to be created before calling this function with ConnectToHost
// It should be closed with closesocket() once all files have been downloaded
void CTournament::DownloadConfig(const char *szURL, SOCKET sock, bool bOverwrite)
{
	// todo: rewrite with https://github.com/corporateshark/LUrlParser
	const char *pFileName = strrchr(szURL, '/');

	char szGameDir[MAX_PATH];
	engine->GetGameDir(szGameDir, sizeof(szGameDir));

	char szFullPath[MAX_PATH];
	char szBakFile[MAX_PATH];
	V_snprintf(szFullPath, sizeof(szFullPath), "%s/cfg%s", szGameDir, pFileName);
	V_snprintf(szBakFile, sizeof(szBakFile), "%s/cfg%s.bak", szGameDir, pFileName);

	if(!bOverwrite && filesystem->FileExists(szFullPath))
		return;

	rename(szFullPath, szBakFile);

	FILE *pConfigFile = fopen(szFullPath, "w");
	if(!pConfigFile)
	{
		Msg("[TFTrue] Failed to open %s\n", szFullPath);
		rename(szBakFile, szFullPath);
		m_iConfigDownloadFailed++;
		return;
	}

	char szURLTemp[70];
	V_strncpy(szURLTemp, szURL, sizeof(szURLTemp));

	char *pFilePath = strchr(szURLTemp, '/');
	pFilePath[0] = '\0';\

	char szPacket[1024];
	V_snprintf(szPacket, sizeof(szPacket), "GET /%s HTTP/2.0\r\n"
										   "Host: %s\r\n"
										   "Accept: */*\r\n\r\n", pFilePath+1, szURLTemp );
										   // let the server control caching
										   //"Cache-Control: max-age=0\r\n\r\n", pFilePath+1, szURLTemp );

	printf("%s\n", szPacket);

	if (send(sock, szPacket, strlen(szPacket), 0) <= 0) // Send the packet
	{
		char Line[255];
		sprintf(Line, "[TFTrue] Failed to download %s, send error\n", szURL);
		Msg("[TFTrue] Failed to download %s, send error\n", szURL);
		engine->LogPrint(Line);

		remove(szFullPath);
		rename(szBakFile, szFullPath);
		m_iConfigDownloadFailed++;
		fclose(pConfigFile);
		return;
	}

	// Now we read the response
	char HeaderField[512] = {};
	char ReadChar;
	int iReadResult = 0;
	int iHTTPCode = 0;
	unsigned int uiContentLength = 0;
	bool bChunkedEncoding = false;

	// Read headers
	for(iReadResult = recv(sock, &ReadChar, sizeof(ReadChar), 0); iReadResult > 0; iReadResult = recv(sock, &ReadChar, sizeof(ReadChar), 0))
	{
		// Read \r\n
		if(ReadChar == '\r' && recv(sock, &ReadChar, sizeof(ReadChar), MSG_PEEK) > 0 && ReadChar == '\n' )
		{
			recv(sock, &ReadChar, sizeof(ReadChar), 0); // Remove \n from the buffer

			if(!HeaderField[0]) // Header already found on previous iteration, we've encountered \r\n\r\n, end of headers
				break;

			sscanf(HeaderField, "HTTP/%*d.%*d %3d", &iHTTPCode);
			sscanf(HeaderField, "Content-Length: %d", &uiContentLength);
			if(!strcmp(HeaderField, "Transfer-Encoding: chunked"))
				bChunkedEncoding = true;

			//Msg("Header: %s\n", HeaderField);
			HeaderField[0] = '\0';
		}
		else
		{
			char NewChar[2]; NewChar[0] = ReadChar; NewChar[1] = '\0';
			strcat(HeaderField, NewChar);
		}
	}

	// Chunked Encoding, we don't have a Content-Length
	if(bChunkedEncoding)
	{
		char ChunkSize[10] = {};

		for(iReadResult = recv(sock, &ReadChar, sizeof(ReadChar), 0); iReadResult > 0; iReadResult = recv(sock, &ReadChar, sizeof(ReadChar), 0))
		{
			// Read \r\n
			if(ReadChar == '\r' && recv(sock, &ReadChar, sizeof(ReadChar), MSG_PEEK) > 0 && ReadChar == '\n' )
			{
				recv(sock, &ReadChar, sizeof(ReadChar), 0); // Remove \n from the buffer

				if(!ChunkSize[0]) // We haven't stored the ChunkSize yet, go back in the loop to read it
					continue;

				uiContentLength = strtoul(ChunkSize, NULL, 16);

				// Length is 0, there are no more chunks
				if(uiContentLength == 0)
				{
					recv(sock, &ReadChar, sizeof(ReadChar), 0); // \r
					recv(sock, &ReadChar, sizeof(ReadChar), 0); // \n
					break;
				}

				// Read content
				char ContentRead[2048] = {};
				int iNumBytesLeft = uiContentLength;

				while(iNumBytesLeft > 0 &&
					  (iReadResult = recv(sock, ContentRead, iNumBytesLeft > sizeof(ContentRead) ? sizeof(ContentRead) : iNumBytesLeft, 0)) > 0)
				{
					fwrite(ContentRead, 1, iReadResult, pConfigFile);
					iNumBytesLeft -= iReadResult;
				}

				ChunkSize[0] = '\0';
			}
			else
			{
				char NewChar[2]; NewChar[0] = ReadChar; NewChar[1] = '\0';
				strcat(ChunkSize, NewChar);
			}
		}
	}
	else
	{
		// Read content
		char ContentRead[2048] = {};
		int iNumBytesRead = 0;

		while(uiContentLength > iNumBytesRead && (iReadResult = recv(sock, ContentRead, sizeof(ContentRead), 0)) > 0)
		{
			fwrite(ContentRead, 1, iReadResult, pConfigFile);
			iNumBytesRead += iReadResult;
		}
	}

	fclose(pConfigFile);

	if (iHTTPCode == 200)
	{
		Msg("[TFTrue] Successfully downloaded %s\n", szURL);
		remove(szBakFile);
	}
	else
	{
		char Line[255];
		sprintf(Line, "[TFTrue] Could not download %s. HTTP error %d\n", szURL, iHTTPCode);
		Msg("[TFTrue] Could not download %s. HTTP error %d\n", szURL, iHTTPCode);
		engine->LogPrint(Line);

		remove(szFullPath);
		rename(szBakFile, szFullPath);
		m_iConfigDownloadFailed++;
	}
}
