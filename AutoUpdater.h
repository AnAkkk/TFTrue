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

#include <string>
#include <stddef.h>
#include <time.h>
#include <vector>
#include <fstream>

#include "SDK.h"

class CAutoUpdater
{
public:
    CAutoUpdater(): m_CallbackHTTPRequestDataReceived(this, &CAutoUpdater::OnHTTPRequestDataReceived) {}
    void CheckUpdate();
    void Init();
    void OnGameFrame();

    std::string GetCurrentModulePath();

private:
    bool IsModuleValid(std::string strFileName );
    void DownloadUpdate(HTTPRequestCompleted_t *arg);
    void FinishUpdate();

    void UpdateCallback(HTTPRequestCompleted_t *arg, bool bFailed);

    CCallResult<CAutoUpdater, HTTPRequestCompleted_t> m_callResult;

    std::string m_strFilePath;
    std::string m_strBakFile;
    std::ofstream m_fNewBin;

    STEAM_GAMESERVER_CALLBACK( CAutoUpdater, OnHTTPRequestDataReceived, HTTPRequestDataReceived_t, m_CallbackHTTPRequestDataReceived);

	enum HttpRequestType {
		CHECK_UPDATE,
		DOWNLOAD_UPDATE
	};
};

extern CAutoUpdater g_AutoUpdater;
