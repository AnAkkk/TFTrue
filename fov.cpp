#include "fov.h"
#include "utils.h"

CFOV g_FOV;

ConVar tftrue_maxfov("tftrue_maxfov", "90", FCVAR_NOTIFY, "Maximum FoV using the !fov command.", true, 75, true, 179, &CFOV::Max_FOV_Callback);

CFOV::CFOV()
{
	for (unsigned int i = 0; i < sizeof(m_iFOV)/sizeof(int); i++)
		m_iFOV[i] = -1;
}

void CFOV::OnLoad()
{
	SendTable *pTable = g_SendProp.GetSendTable("CTFPlayer");
	if(pTable)
	{
		pProp = g_SendProp.GetSendProp(pTable, "m_iFOV");
		oldProxy = pProp->GetProxyFn();
		pProp->SetProxyFn(Proxy);
	}
}

void CFOV::OnUnload()
{
	if(pProp)
		pProp->SetProxyFn(oldProxy);
}

void CFOV::OnPlayerDisconnect(edict_t *pEntity)
{
	m_iFOV[IndexOfEdict(pEntity)-1] = -1;
}

void CFOV::OnFOVCommand(unsigned int uiFOV)
{
	if (uiFOV >= 75 && (int)uiFOV <= tftrue_maxfov.GetInt())
	{
		*g_EntityProps.GetSendProp<int>(CBaseEntity::Instance(g_Plugin.GetCommandIndex()+1), "m_iFOV") = uiFOV;
		m_iFOV[g_Plugin.GetCommandIndex()] = uiFOV;
	}
	else
	{
		char Line[255];
		sprintf(Line, "\003Usage : !fov [75-%u]", tftrue_maxfov.GetInt());
		Message(g_Plugin.GetCommandIndex()+1, Line);
	}
}

void CFOV::OnClientSettingsChanged(edict_t *pEdict)
{
	int iEntIndex = IndexOfEdict(pEdict);

	const char *szWantedFOV = engine->GetClientConVarValue(iEntIndex, "tftrue_fov");

	if(strcmp(szWantedFOV, ""))
	{
		int iWantedFOV = atoi(szWantedFOV);
		if(iWantedFOV != m_iFOV[iEntIndex-1])
		{
			iWantedFOV = clamp(iWantedFOV, 75, tftrue_maxfov.GetInt());

			CBasePlayer *pPlayer = (CBasePlayer*)CBaseEntity::Instance(pEdict);
			if(pPlayer)
				*g_EntityProps.GetSendProp<int>(pPlayer, "m_iFOV") = iWantedFOV;
		}
	}
}

void CFOV::Proxy( const SendProp *pProp, const void *pStructBase, const void *pData, DVariant *pOut, int iElement, int objectID )
{
	int fovChange = *(int *)pData;
	int fovChosen = g_FOV.m_iFOV[objectID-1];

	if(fovChosen != -1) // player chose a custom fov
	{
		if(!fovChange) // fov was reset
			pOut->m_Int = fovChosen;
		else
			pOut->m_Int = fovChange;
	}
	else
		pOut->m_Int = fovChange;
}

void CFOV::Max_FOV_Callback( IConVar *var, const char *pOldValue, float flOldValue )
{
	ConVar* v = (ConVar*)var;
	for ( int i = 1; i <= g_pServer->GetClientCount(); i++ )
	{
		CBasePlayer *pPlayer = (CBasePlayer*)CBaseEntity::Instance(i);
		if(!pPlayer)
			continue;

		if(engine->GetPlayerNetInfo(pPlayer->entindex())) // Not a bot
		{
			int iCurrentFOV = *g_EntityProps.GetSendProp<int>(pPlayer, "m_iFOV");
			if(iCurrentFOV != 0)
			{
				if(iCurrentFOV > v->GetInt())
					*g_EntityProps.GetSendProp<int>(pPlayer, "m_iFOV") = v->GetInt();
				else if(iCurrentFOV < 75)
					*g_EntityProps.GetSendProp<int>(pPlayer, "m_iFOV") = 75;
			}
		}
	}
}
