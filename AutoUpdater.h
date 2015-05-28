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

class CAutoUpdater
{
public:
	static bool StartAutoUpdater();
	void OnGameFrame();

	std::string GetCurrentModulePath();

private:
	bool IsModuleValid(std::string strFileName );
	bool GetFileMD5(std::string strFileName, char ucMD5[] );
	void Base64ToHex(const char *szBase64, size_t length, char *szResult );

	int m_iStatus = 0;

	time_t m_tLastMessageTime = 0;
};

extern CAutoUpdater g_AutoUpdater;
