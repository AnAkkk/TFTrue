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

#include "AutoUpdater.h"
#include "TFTrue.h"
#include "fov.h"
#include "items.h"
#include "bunnyhop.h"
#include "tournament.h"
#include "sourcetv.h"
#include "logs.h"
#include "utils.h"
#include "stats.h"

#include <thread>

#ifdef WIN32
#include <WS2tcpip.h>
#endif

CAutoUpdater g_AutoUpdater;

#ifndef _LINUX
    #define DOWNLOAD_URL "https://tftrue.esport-tools.net/TFTrue.dll"
#else
    #define DOWNLOAD_URL "https://tftrue.esport-tools.net/TFTrue.so"
#endif

void CAutoUpdater::Init()
{
    m_strFilePath = GetCurrentModulePath(); // Path + File name
    m_strBakFile = m_strFilePath + ".bak"; // Path + File name with bak extension
    remove(m_strBakFile.c_str());
}

std::string CAutoUpdater::GetCurrentModulePath()
{
#ifdef WIN32
	char szFilePath[MAX_PATH];
	MEMORY_BASIC_INFORMATION mbi;
	static int dummy;
	VirtualQuery( &dummy, &mbi, sizeof(mbi) );
	GetModuleFileName((HMODULE)mbi.AllocationBase, szFilePath, sizeof(szFilePath));
	return std::string(szFilePath);
#else
	Dl_info info;
	dladdr(&g_Plugin, &info);
	return std::string(info.dli_fname);
#endif
}

bool CAutoUpdater::IsModuleValid(std::string strFileName)
{
	CSysModule *module = Sys_LoadModule(strFileName.c_str());
	if(module)
	{
		Sys_UnloadModule(module);
		return true;
	}
	return false;
}

void CAutoUpdater::OnGameFrame()
{
	// If there is only one player left or none playing on the server, we can update
    if(g_pServer->GetNumPlayers() <= 1 && steam.SteamHTTP())
	{
		static time_t tLastCheckTime = time(NULL);
		// Only every hour
		if(time(NULL) - tLastCheckTime > 3600)
		{
			tLastCheckTime = time(NULL);

            CheckUpdate();
		}
	}
}

bool CAutoUpdater::GetFileMD5(std::string strFileName, char ucMD5[33])
{
	unsigned char ucDigest[16];

	MD5Context_t ctx;

	MD5Init(&ctx);

	std::ifstream f;
	f.open(strFileName, std::ifstream::binary);
	if(!f.is_open())
		return false;

	while( !f.eof() )
	{
		unsigned char ucBuffer[1024];
		f.read((char*)ucBuffer, 1024);
		MD5Update(&ctx, ucBuffer, f.gcount());
	}

	f.close();

	MD5Final(ucDigest, &ctx);

	for (int i = 0; i < 16; i++)
		sprintf( ucMD5 + i*2, "%02x", ucDigest[i] );

	return true;
}

void CAutoUpdater::Base64ToHex( const char *szBase64, size_t length, char *szResult )
{
	unsigned int uiHexPos = 0;
	unsigned char *ucHex = new unsigned char[length/4*3];

	unsigned int uiNumSkip = 0;

	for(unsigned int i = 0; i < length; i+=4)
	{
		char a = GetBase64Value(szBase64[i]);
		char b = GetBase64Value(szBase64[i+1]);
		char c = GetBase64Value(szBase64[i+2]);
		char d = GetBase64Value(szBase64[i+3]);

		int result = 0;

		if(c == -1 && d == -1) // Check if we have ==
		{
			result = (a << 18) + (b << 12);
			ucHex[uiHexPos+1] = '\0';
			ucHex[uiHexPos+2] = '\0';
			uiNumSkip = 2;
		}
		else if( d == -1 ) // Check if we have =
		{
			result = (a << 18) + (b << 12) + (c << 6);
			ucHex[uiHexPos+1] = (result >> 8) & 0xFF;
			ucHex[uiHexPos+2] = '\0';
			uiNumSkip = 1;
		}
		else
		{
			result = (a << 18) + (b << 12) + (c << 6) + d;
			ucHex[uiHexPos+1] = (result >> 8) & 0xFF;
			ucHex[uiHexPos+2] = result & 0xFF;
		}

		ucHex[uiHexPos] = result >> 16;

		uiHexPos += 3;
	}

	for (unsigned int i = 0; i < length/4*3 - uiNumSkip; i++)
		sprintf( szResult + i*2, "%02x", ucHex[i] );

	delete[] ucHex;
}

void CAutoUpdater::DownloadUpdate(HTTPRequestCompleted_t *arg)
{
    Msg("[TFTrue] Updating...\n");

    if(rename(m_strFilePath.c_str(), m_strBakFile.c_str()))
    {
        Msg("[TFTrue] Error while renaming the binary\n");
        return;
    }

    m_fNewBin.open(m_strFilePath, std::ofstream::binary);

    if(!m_fNewBin.is_open())
    {
        Msg("[TFTrue] Failed to open the binary\n");
        rename(m_strBakFile.c_str(), m_strFilePath.c_str());
        return;
    }

    SteamAPICall_t hCallServer;
    HTTPRequestHandle handle = steam.SteamHTTP()->CreateHTTPRequest(k_EHTTPMethodGET, DOWNLOAD_URL);
    steam.SteamHTTP()->SetHTTPRequestHeaderValue(handle, "Cache-Control", "no-cache");
    steam.SteamHTTP()->SetHTTPRequestContextValue(handle, DOWNLOAD_UPDATE);
    steam.SteamHTTP()->SendHTTPRequestAndStreamResponse(handle, &hCallServer);

    m_callResult.SetGameserverFlag();
    m_callResult.Set(hCallServer, this, &CAutoUpdater::UpdateCallback);
}


void CAutoUpdater::FinishUpdate()
{
    if(m_fNewBin.is_open())
        m_fNewBin.close();

    char szNewMD5[33] = "";
    GetFileMD5(m_strFilePath, szNewMD5);

    if(!IsModuleValid(m_strFilePath) || strcmp(szNewMD5, m_szExpectedMD5))
    {
        Msg("[TFTrue] The new binary is corrupted!\n");
        unlink(m_strFilePath.c_str());
        rename(m_strBakFile.c_str(), m_strFilePath.c_str());
        return;
    }

    int iPluginIndex = -1;

    if(helpers)
    {
        CUtlVector<CPlugin *> *m_Plugins = (CUtlVector<CPlugin *>*)((char*)helpers+4);
        for ( int i = 0; i < m_Plugins->Count(); i++ )
        {
            if(!strncmp(m_Plugins->Element(i)->m_szName, "TFTrue", 6))
            {
                iPluginIndex = i;
                break;
            }
        }
    }

    // No plugin index, something is wrong
    if(iPluginIndex == -1)
    {
        Msg("[TFTrue] Could not find plugin index to reload it\n");
        return;
    }

    Msg("[TFTrue] Reloading plugin due to update\n");

    char szGameDir[1024];
    engine->GetGameDir(szGameDir, sizeof(szGameDir));

    // Reload the plugin
    std::string strPluginReload;
    strPluginReload.append("plugin_unload ").append(std::to_string(iPluginIndex)).append(";plugin_load ").append(m_strFilePath.c_str()+strlen(szGameDir)+1).append("\n");
    engine->InsertServerCommand(strPluginReload.c_str());

    // Restore cvar values
    CVarDLLIdentifier_t id = tftrue_version.GetDLLIdentifier();
    for(ConCommandBase *pCommand = g_pCVar->GetCommands(); pCommand; pCommand = pCommand->GetNext())
    {
        if(pCommand->GetDLLIdentifier() != id)
            continue;

        if(pCommand->IsCommand())
            continue;

        ConVar *pVar = (ConVar*)pCommand;

        // Ignore tftrue_version
        if(pVar->IsFlagSet(FCVAR_CHEAT))
            continue;

        // Ignore cvars with no values
        if(strcmp(pVar->GetString(), "") == 0)
            continue;

        std::string strCommandBackup;
        strCommandBackup.append(pVar->GetName()).append(" ").append(pVar->GetString()).append("\n");
        engine->InsertServerCommand(strCommandBackup.c_str());
    }
}

void CAutoUpdater::UpdateCallback(HTTPRequestCompleted_t *arg, bool bFailed)
{
    if(bFailed || arg->m_eStatusCode < 200 || arg->m_eStatusCode > 299)
    {
        uint32 size;
        steam.SteamHTTP()->GetHTTPResponseBodySize(arg->m_hRequest, &size);

        if(size > 0)
        {
            uint8 *pResponse = new uint8[size+1];
            steam.SteamHTTP()->GetHTTPResponseBodyData(arg->m_hRequest, pResponse, size);
            pResponse[size] = '\0';

            Msg("[TFTrue] The update data hasn't been received. HTTP error %d. Response: %s\n", arg->m_eStatusCode, pResponse);

            delete[] pResponse;
        }
        else if(!arg->m_bRequestSuccessful)
        {
            Msg("[TFTrue] The update data hasn't been received. No response from the server.\n");
        }
        else
        {
            Msg("[TFTrue] The update data hasn't been received. HTTP error %d\n", arg->m_eStatusCode);
        }
    }
    else if(arg->m_ulContextValue == CHECK_UPDATE)
    {
        char szContentMD5[25] = {0};
        if(steam.SteamHTTP()->GetHTTPResponseHeaderValue(arg->m_hRequest, "Content-MD5", szContentMD5, sizeof(szContentMD5)))
        {
            char szCurrentMD5[33] = "";

            Base64ToHex(szContentMD5, strlen(szContentMD5), m_szExpectedMD5);
            GetFileMD5(m_strFilePath, szCurrentMD5);

            if(!strcmp(m_szExpectedMD5, szCurrentMD5))
            {
                Msg("[TFTrue] The plugin is up to date!\n");
            }
            else
            {
                DownloadUpdate(arg);
            }
        }
    }
    else if(arg->m_ulContextValue == DOWNLOAD_UPDATE)
    {
        FinishUpdate();
    }

    steam.SteamHTTP()->ReleaseHTTPRequest(arg->m_hRequest);
}

void CAutoUpdater::OnHTTPRequestDataReceived(HTTPRequestDataReceived_t *pParam)
{
    if(!m_fNewBin.is_open())
    {
        Msg("[TFTrue] Failed to open the new binary\n");
        return;
    }

    // Download the new binary
    uint8 *pResponse = new uint8[pParam->m_cBytesReceived];
    if(steam.SteamHTTP()->GetHTTPStreamingResponseBodyData(pParam->m_hRequest, pParam->m_cOffset, pResponse, pParam->m_cBytesReceived))
        m_fNewBin.write((const char*)pResponse, pParam->m_cBytesReceived);

    delete[] pResponse;
}

void CAutoUpdater::CheckUpdate()
{
    SteamAPICall_t hCallServer;
    HTTPRequestHandle handle = steam.SteamHTTP()->CreateHTTPRequest(k_EHTTPMethodHEAD, DOWNLOAD_URL);
    steam.SteamHTTP()->SetHTTPRequestHeaderValue(handle, "Cache-Control", "no-cache");
    steam.SteamHTTP()->SetHTTPRequestContextValue(handle, CHECK_UPDATE);
    steam.SteamHTTP()->SendHTTPRequest(handle, &hCallServer);

    m_callResult.SetGameserverFlag();
    m_callResult.Set(hCallServer, this, &CAutoUpdater::UpdateCallback);
}
