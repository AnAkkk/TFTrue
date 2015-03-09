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

class CFOV
{
public:
	CFOV();

	void OnLoad();
	void OnUnload();
	void OnPlayerDisconnect(edict_t *pEntity);
	void OnClientSettingsChanged( edict_t *pEdict );
	void OnFOVCommand(unsigned int uiFOV);

	static void Proxy( const SendProp *pProp, const void *pStructBase, const void *pData, DVariant *pOut, int iElement, int objectID );
	static void Max_FOV_Callback( IConVar *var, const char *pOldValue, float flOldValue );
private:
	int m_iFOV[34];
	SendProp *pProp;
	SendVarProxyFn oldProxy;
};

extern CFOV g_FOV;

extern ConVar tftrue_maxfov;
