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

#include "valve_minmax_off.h"
#include <mutex>
#include <map>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr SOCKADDR;
#define INVALID_SOCKET -1
#endif

class CTFTrue: public IServerPluginCallbacks
{
public:
	// IServerPluginCallbacks methods
	virtual bool			Load(	CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory );
	virtual void			Unload( void );
	virtual void			Pause( void ) {}
	virtual void			UnPause( void ) {}
	virtual const char     *GetPluginDescription( void );
	virtual void			LevelInit( char const *pMapName ){}
	virtual void			ServerActivate( edict_t *pEdictList, int edictCount, int clientMax );
	virtual void			GameFrame( bool simulating );
	virtual void			LevelShutdown( void ){}
	virtual void			ClientActive( edict_t *pEntity ) { }
	virtual void			ClientDisconnect( edict_t *pEntity );
	virtual void			ClientPutInServer( edict_t *pEntity, char const *playername ){}
	virtual void			SetCommandClient( int index );
	virtual void			ClientSettingsChanged( edict_t *pEdict );
	virtual PLUGIN_RESULT	ClientConnect( bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen ) {return PLUGIN_CONTINUE;}
	virtual PLUGIN_RESULT	ClientCommand( edict_t *pEntity, const CCommand &args );
	virtual PLUGIN_RESULT	NetworkIDValidated( const char *pszUserName, const char *pszNetworkID ) {return PLUGIN_CONTINUE;}
	virtual void			OnQueryCvarValueFinished( QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue ){}
	virtual void			OnEdictAllocated( edict_t *edict ){}
	virtual void			OnEdictFreed( const edict_t *edict  ){}

	int GetCommandIndex() { return m_iClientCommandIndex; }

	void ForceReloadMap(float flTime = 0.0f);

	void UpdateGameDesc();

	static void Version_Callback( IConVar *var, const char *pOldValue, float flOldValue );
	static void GameDesc_Callback( IConVar *var, const char *pOldValue, float flOldValue );
	static void Freezecam_Callback( IConVar *var, const char *pOldValue, float flOldValue );

	edict_t *m_pEdictList;

	void ForwardCommand(ConCommand *pCmd, const CCommand &args);

	void *GetGameRules() { return m_pGameRulesData; }

private:
	bool m_bReloadedNeeded = false;
	int m_iLoadCount = 0;
	int m_iClientCommandIndex = 0;

	bool m_bForceReloadMap = false;
	float m_flNextReloadMap = 0.0f;

	char m_szGameDesc[50] = {};

	void *m_pGameRulesData = nullptr;

	static const char* __fastcall GetGameDescription(IServerGameDLL *gamedll EDX2);
	CFunctionRoute m_GetGameDescriptionRoute;

	static void __fastcall ChangeLevel(IVEngineServer *pServer, EDX const char *s1, const char *s2);
	CFunctionRoute m_ChangeLevelRoute;

	static void __fastcall Say_Callback(ConCommand *pCmd, EDX const CCommand &args);
	CFunctionRoute m_DispatchSayRoute;

	static void __fastcall GameServerSteamAPIActivated(IServerGameDLL *gamedll EDX2);
	CFunctionRoute m_GameServerSteamAPIActivatedRoute;
};

extern CTFTrue g_Plugin;

#ifdef _LINUX
#define GetFuncAddress(pAddress, szFunction) dlsym(pAddress, szFunction)
#else
#define GetFuncAddress(pAddress, szFunction) ::GetProcAddress((HMODULE)pAddress, szFunction)
#endif

extern ConVar tftrue_gamedesc;
extern ConVar tftrue_freezecam;
extern ConVar tftrue_version;
