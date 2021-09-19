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

#include <json/json.h>

CAutoUpdater g_AutoUpdater;

#define UPDATE_URL "https://api.github.com/repos/AnAkkk/TFTrue/releases/latest"

#ifndef _LINUX
    #define DOWNLOAD_URL "https://raw.githubusercontent.com/AnAkkk/TFTrue/public/release/TFTrue.dll"
#else
    #define DOWNLOAD_URL "https://raw.githubusercontent.com/AnAkkk/TFTrue/public/release/TFTrue.so"
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

    if(!IsModuleValid(m_strFilePath))
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
        uint32 size;
        steam.SteamHTTP()->GetHTTPResponseBodySize(arg->m_hRequest, &size);

        if(size > 0)
        {
            uint8 *pResponse = new uint8[size+1];
            steam.SteamHTTP()->GetHTTPResponseBodyData(arg->m_hRequest, pResponse, size);
            pResponse[size] = '\0';

            std::string strResponse((char*)pResponse);
            Json::Value root;
            Json::Reader reader;

            bool parsingSuccessful = reader.parse(strResponse, root);
            if(!parsingSuccessful)
            {
                Msg("[TFTrue] Failed to parse update info.\n");
            }
            else
            {
                const Json::Value tagName = root["tag_name"];
                if(tagName.isString())
                {
                    std::string strTagName = tagName.asString();
                    float tagVersion = roundf(atof(strTagName.c_str()) * 100) / 100;
                    float currentVersion = roundf(tftrue_version.GetFloat() * 100) / 100;
                    if(tagVersion <= currentVersion)
                    {
                        Msg("[TFTrue] The plugin is up to date!\n");
                    }
                    else
                    {
                        DownloadUpdate(arg);
                    }
                }
            }

            delete[] pResponse;
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
    HTTPRequestHandle handle = steam.SteamHTTP()->CreateHTTPRequest(k_EHTTPMethodGET, UPDATE_URL);
    steam.SteamHTTP()->SetHTTPRequestHeaderValue(handle, "Cache-Control", "no-cache");
    steam.SteamHTTP()->SetHTTPRequestContextValue(handle, CHECK_UPDATE);
    steam.SteamHTTP()->SendHTTPRequest(handle, &hCallServer);

    m_callResult.SetGameserverFlag();
    m_callResult.Set(hCallServer, this, &CAutoUpdater::UpdateCallback);
}
