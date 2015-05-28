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

#include <fstream>
#include <thread>

#ifdef WIN32
#include <WS2tcpip.h>
#endif

CAutoUpdater g_AutoUpdater;

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
	if(g_pServer->GetNumPlayers() <= 1)
	{
		static time_t tLastCheckTime = time(NULL);
		// Only every hour
		if(time(NULL) - tLastCheckTime > 3600)
		{
			tLastCheckTime = time(NULL);

			std::thread AutoUpdaterThread(&CAutoUpdater::StartAutoUpdater);
			AutoUpdaterThread.detach();
		}
	}

	if(m_iStatus)
	{
		if(m_iStatus == -2)
		{
			if(time(NULL) - m_tLastMessageTime > 1800)
			{
				AllMessage("\003[TFTrue] Auto Updater failed, please contact AnAkkk to report this issue\n");
				m_tLastMessageTime = time(NULL);
			}
		}
		else if(m_iStatus >= 10000)
		{
			if(time(NULL) - m_tLastMessageTime > 3600)
			{
				AllMessage("\003[TFTrue] An update is available but it could not be downloaded. Error: %d\n", m_iStatus - 10000);
				m_tLastMessageTime = time(NULL);
			}
		}
		else
		{
			if(time(NULL) - m_tLastMessageTime > 3600)
			{
				AllMessage("\003[TFTrue] Auto Updater failed. Error: %d\n", m_iStatus);
				m_tLastMessageTime = time(NULL);
			}
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

bool CAutoUpdater::StartAutoUpdater()
{
	std::string strFilePath = g_AutoUpdater.GetCurrentModulePath(); // Path + File name

	if(strFilePath.length() + sizeof(".bak") > MAX_PATH)
	{
		Msg("[TFTrue] Can't autoupdate: path length is too long.\n");
		engine->LogPrint("[TFTrue] Can't autoupdate: path length is too long.\n");
		g_AutoUpdater.m_iStatus = -2;
		return false;
	}

	std::string strBakFile = strFilePath + ".bak"; // Path + File name with bak extension
	remove(strBakFile.c_str());

	SOCKET sock = INVALID_SOCKET;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock == INVALID_SOCKET)
	{
		Msg("[TFTrue] Failed to create socket.\n");
		engine->LogPrint("[TFTrue] Failed to create socket.\n");
		g_AutoUpdater.m_iStatus = -2;
		return false;
	}

	// Set the socket to non blocking, so we don't block on connect() but instead on select()
#ifdef WIN32
	u_long iMode = 1;
	ioctlsocket(sock, FIONBIO, &iMode);
#else
	int flags;
	flags = fcntl(sock, F_GETFL);
	fcntl(sock, F_SETFL, flags|O_NONBLOCK);
#endif

	const char *szHostName = "tftrue.esport-tools.net";

	struct addrinfo *result;
	struct addrinfo hints;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_NUMERICSERV;

	if(getaddrinfo(szHostName, "80", nullptr, &result) != 0)
	{
		char Line[255];
		sprintf(Line, "[TFTrue] Failed to resolve %s\n", szHostName);
		Msg("[TFTrue] Failed to resolve %s\n", szHostName);
		engine->LogPrint(Line);
		closesocket(sock);
		g_AutoUpdater.m_iStatus = -2;
		return false;
	}

#ifdef WIN32
	int timeout = 5000;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(int));
	setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(int));
#else
	struct timeval timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;

	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval));
	setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(struct timeval));
#endif

	if(connect(sock, result->ai_addr, result->ai_addrlen) == -1)
	{
		struct timeval tv;
		tv.tv_sec = 5;
		tv.tv_usec = 0;

		fd_set writefds;
		FD_ZERO(&writefds);
		FD_SET(sock, &writefds);

		// Block until we've finished connecting or the timeout expired
		if(select(sock+1, NULL, &writefds, NULL, &tv) > 0)
		{
			int length = sizeof(int);
			int value;
#ifdef WIN32
			getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&value, &length);
#else
			getsockopt(sock, SOL_SOCKET, SO_ERROR, &value, (socklen_t *__restrict)&length);
#endif
			// No error, set back the socket to blocking
			if (!value)
			{
#ifdef WIN32
				iMode = 0;
				ioctlsocket(sock, FIONBIO, &iMode);
#else
				flags = fcntl(sock, F_GETFL, NULL);
				fcntl(sock, F_SETFL, flags &= (~O_NONBLOCK));
#endif
			}
			else
			{
				char Line[255];
				sprintf(Line, "\003[TFTrue] Failed to connect to %s: %d\n", szHostName, value);
				Msg("[TFTrue] Failed to connect to %s: %d\n", szHostName, value);
				engine->LogPrint(Line);
				closesocket(sock);
				g_AutoUpdater.m_iStatus = -2;
				return false;
			}
		}
		else
		{
			char Line[255];
			sprintf(Line, "[TFTrue] Timed out while connecting to %s\n", szHostName);
			Msg("[TFTrue] Timed out while connecting to %s\n", szHostName);
			engine->LogPrint(Line);
			closesocket(sock);
			g_AutoUpdater.m_iStatus = -2;
			return false;
		}
	}
	else
	{
		char Line[255];
		sprintf(Line, "[TFTrue] Failed to connect to %s\n", szHostName);
		Msg("[TFTrue] Failed to connect to %s\n", szHostName);
		engine->LogPrint(Line);
		closesocket(sock);
		g_AutoUpdater.m_iStatus = -2;
		return false;
	}

	std::string strPacket;
	strPacket.reserve(1024);

#ifndef _LINUX
	const char *szBinURL = "/TFTrue.dll";
#else
	const char *szBinURL = "/TFTrue.so";
#endif

	strPacket.append("GET ").append(szBinURL).append(" HTTP/1.1\r\n");
	strPacket.append("Host: ").append(szHostName).append("\r\n");
	strPacket.append("Accept: */*\r\n");
	strPacket.append("Cache-Control: no-cache\r\n");
	strPacket.append("User-Agent: TFTrue Auto Updater\r\n\r\n");

	if(send(sock, strPacket.c_str(), strPacket.length(), 0) <= 0) // Send the packet
	{
		char Line[] = "[TFTrue] Failed to download the new version, send error\n";
		Msg("[TFTrue] Failed to download the new version, send error\n");
		engine->LogPrint(Line);
		closesocket(sock);
		g_AutoUpdater.m_iStatus = 10000;
		return false;
	}

	// Now we read the response
	char HeaderField[512] = {};
	char ReadChar = 0;
	int iReadResult = 0;
	int iHTTPCode = 0;
	unsigned int uiContentLength = 0;
	char szContentMD5[25];

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
			sscanf(HeaderField, "Content-MD5: %s", szContentMD5);

			//Msg("Header: %s\n", HeaderField);
			HeaderField[0] = '\0';
		}
		else
		{
			char NewChar[2]; NewChar[0] = ReadChar; NewChar[1] = '\0';
			strcat(HeaderField, NewChar);
		}
	}

	if(iHTTPCode != 200)
	{
		Msg("[TFTrue] Error while downloading new version! Error: %d\n", iHTTPCode);
		g_AutoUpdater.m_iStatus = iHTTPCode + 10000;
		closesocket(sock);
		return false;
	}

	char szMD5[33] = "";
	char szCurrentMD5[33] = "";

	g_AutoUpdater.Base64ToHex(szContentMD5, strlen(szContentMD5), szMD5);
	g_AutoUpdater.GetFileMD5(strFilePath, szCurrentMD5);

	if(!strcmp(szMD5, szCurrentMD5))
	{
		Msg("[TFTrue] The plugin is up to date!\n");
		closesocket(sock);
		return false;
	}

	Msg("[TFTrue] Updating...\n");

	if(rename(strFilePath.c_str(), strBakFile.c_str()))
	{
		Msg("[TFTrue] Error while renaming the binary\n");
		g_AutoUpdater.m_iStatus = 10001;
		closesocket(sock);
		return false;
	}

	std::ofstream fNewBin;
	fNewBin.open(strFilePath, std::ofstream::binary);

	if(!fNewBin.is_open())
	{
		Msg("[TFTrue] Failed to open the binary\n");
		rename(strBakFile.c_str(), strFilePath.c_str());
		g_AutoUpdater.m_iStatus = 10002;
		closesocket(sock);
		return false;
	}

	// Read content
	char ContentRead[2048] = {};
	int iNumBytesRead = 0;

	while(uiContentLength > iNumBytesRead && (iReadResult = recv(sock, ContentRead, sizeof(ContentRead), 0)) > 0)
	{
		fNewBin.write(ContentRead, iReadResult);
		iNumBytesRead += iReadResult;
	}

	fNewBin.close();
	closesocket(sock);

	char szNewMD5[33] = "";
	g_AutoUpdater.GetFileMD5(strFilePath, szNewMD5);

	if(!g_AutoUpdater.IsModuleValid(strFilePath) || strcmp(szNewMD5, szMD5))
	{
		Msg("[TFTrue] The new binary is corrupted!\n");
		unlink(strFilePath.c_str());
		rename(strBakFile.c_str(), strFilePath.c_str());
		g_AutoUpdater.m_iStatus = 10003;
		return false;
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

	// No plugin index, we aren't loaded yet, reload the plugin
	if(iPluginIndex == -1)
		return true;

	char szGameDir[1024];
	engine->GetGameDir(szGameDir, sizeof(szGameDir));

	// Reload the plugin
	std::string strPluginReload;
	strPluginReload.append("plugin_unload ").append(std::to_string(iPluginIndex)).append(";plugin_load ").append(strFilePath.c_str()+strlen(szGameDir)+1).append("\n");
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

	return false;
}
