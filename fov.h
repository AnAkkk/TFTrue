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
