#pragma once

#include "SDK.h"

#include "valve_minmax_off.h"
#include "ModuleScanner.h"

class CItems
{	
public:
	CItems();
	~CItems();
	bool Init(const CModuleScanner& ServerModule);
	void OnUnload();
	
	char *GetAttributeValue(KeyValues *pKItem, const char *szAttribute);
	
	static void RebuildWhitelist(IConVar *var, const char *pOldValue, float flOldValue);
	static void TournamentWhitelistCallback(IConVar *var, const char *pOldValue, float flOldValue);
	
	KeyValues *item_whitelist = nullptr;
	KeyValues *item_schema = nullptr;
	char szWhiteListChosen[MAX_PATH];

	char *GetWhitelistName() { return szWhiteListName; }

	const char* GetItemLogName(int iDefIndex);

private:
	char szWhiteListName[50];

	void *ReloadWhitelist = nullptr;
	void *ItemSystem = nullptr;
	void *GiveDefaultItems = nullptr;
	void (__thiscall* RemoveWearable)(void *pPlayer, void *pWearable) = nullptr;
	void *GetItemDefinition = nullptr;

	typedef void (__thiscall* ReloadWhitelistFn)( void *pEconItemSystem);
	typedef void *(*ItemSystemFn)( void );
	typedef void (__thiscall* GiveDefaultItemsFn)( void* pEnt );
	typedef void* (__thiscall* GetItemDefinitionFn)(void *thisPtr, int iDefIndex);
};

extern CItems g_Items;

extern ConVar tftrue_no_hats;
extern ConVar tftrue_no_misc;
extern ConVar tftrue_no_action;
extern ConVar tftrue_whitelist;
extern ConVar tftrue_whitelist_id;
