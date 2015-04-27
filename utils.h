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

#include "TFTrue.h"

#include "SDK.h"

#ifdef _LINUX
#include <sys/mman.h>
#endif

#include <map>

int UTIL_FindInDataMap(datamap_t *pMap, const char *name);

class CEntityProps
{
public:
	template <typename T>
	T* GetSendProp(CBaseEntity *pEntity, const char *szName);
	template <typename T>
	T* GetDataMapProp(CBaseEntity *pEntity, const char *szName);

private:
	std::map<const char*, unsigned int> m_dataMapProps;
	std::map<const char*, unsigned int> m_sendProps;
};

extern CEntityProps g_EntityProps;

class CSendProp
{
public:
	SendTable* GetSendTable( const char *szTableName );
	int GetSendPropOffset(SendTable *pTable, const char *szPropName, bool bShouldSplit = true);
	SendProp* GetSendProp(SendTable *pTable, const char *szPropName, bool bShouldSplit = true);
	//	SendProp* GetSubClassSendTable(SendTable *pTable, const char *szTableName);
};

extern CSendProp g_SendProp;

template <typename T>
inline T* CEntityProps::GetSendProp(CBaseEntity *pEntity, const char *szName)
{
	auto it = m_sendProps.find(szName);
	if(it != m_sendProps.end())
		return (T*)((char*)pEntity + it->second);

	ServerClass *pClass = pEntity->GetServerClass();
	if(!pClass)
		return nullptr;

	SendTable *pTable = pClass->m_pTable;
	if(!pTable)
		return nullptr;

	int offset = g_SendProp.GetSendPropOffset(pTable, szName);
	if(offset != -1)
		m_sendProps[szName] = offset;
	else
		return nullptr;

	return (T*)((char*)pEntity+offset);
}

template <typename T>
inline T* CEntityProps::GetDataMapProp(CBaseEntity *pEntity, const char *szName)
{
	auto it = m_dataMapProps.find(szName);
	if(it != m_dataMapProps.end())
		return (T*)((char*)pEntity + it->second);

	datamap_t *dm = pEntity->GetDataDescMap();
	if(!dm)
		return nullptr;

	int offset = UTIL_FindInDataMap(dm, szName);

	if(offset != -1)
		m_dataMapProps[szName] = offset;
	else
		return nullptr;

	return (T*)((char*)pEntity+offset);
}

// Messages stuff
int GetMessageType(const char * MessageName);
void TextMessage(int ClientIndex, const char *szMessage, ...);
void Message(int ClientIndex, const char *szMessage, ...);
void AllMessage(int iClientIndex, const char *szMessage, ...);
void AllMessage(const char *szMessage, ...);
void ShowHTMLMOTD(int iClientIndex, const char *szTitle, const char *szURL);

int UTIL_FindInDataMap(datamap_t *pMap, const char *name);

int GetEntIndexFromUserID(int iUserID);

extern SpewOutputFunc_t g_OldSpewOutputFunc;

SpewRetval_t TFTrueSpew( SpewType_t spewType, const tchar *pMsg );

unsigned char *PatchAddress(const void *pAddress, unsigned int iOffset, unsigned int iNumBytes, unsigned char* strBytes);

void PrintTFTrueInfos(edict_t *pEntity);

inline int IndexOfEdict(const edict_t* pEdict)
{
	return pEdict - g_Plugin.m_pEdictList;
}

inline edict_t* EdictFromIndex(int iEdictNum)
{
	edict_t *pEdict = g_Plugin.m_pEdictList + iEdictNum;
	if (pEdict->IsFree())
		return NULL;
	return pEdict;
}

inline int IndexOfEntity(CBaseEntity* pEntity)
{
	return IndexOfEdict(pEntity->edict());
}

bool ConnectToHost(const char *szHostName, SOCKET &retsock);

char GetBase64Value(char c);

#ifdef _WIN32
DWORD GetModuleLoadCount(HMODULE hmod);
#endif
