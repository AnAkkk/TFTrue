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

#include "items.h"
#include "utils.h"
#include "tournament.h"

CItems g_Items;

ConVar tftrue_no_hats("tftrue_no_hats", "1", FCVAR_NOTIFY, "Activate/Desactivate hats.",
					  true, 0, true, 1, CItems::RebuildWhitelist);
ConVar tftrue_no_misc("tftrue_no_misc", "0", FCVAR_NOTIFY, "Activate/Desactivate misc items.",
					  true, 0, true, 1, CItems::RebuildWhitelist);
ConVar tftrue_no_action("tftrue_no_action", "0", FCVAR_NOTIFY, "Activate/Desactivate action items.",
						true, 0, true, 1, CItems::RebuildWhitelist);
ConVar tftrue_whitelist_id("tftrue_whitelist_id", "-1", FCVAR_NOTIFY, "ID of the whitelist to use from whitelist.tf", CItems::RebuildWhitelist);

CItems::CItems()
{
	V_strncpy(szWhiteListName, "error", sizeof(szWhiteListName));
	memset(szWhiteListChosen, 0, sizeof(szWhiteListChosen));

	item_whitelist = new KeyValues("item_whitelist");
	item_schema = new KeyValues("");
}

CItems::~CItems()
{
	item_whitelist->deleteThis();
	item_schema->deleteThis();
}

bool CItems::Init(const CModuleScanner& ServerModule)
{
	item_schema->LoadFromFile(filesystem, "scripts/items/items_game.txt", "MOD");

#ifdef _LINUX
	void *GetLoadoutItem = ServerModule.FindSymbol("_ZN9CTFPlayer14GetLoadoutItemEiib");
	ReloadWhitelist = ServerModule.FindSymbol("_ZN15CEconItemSystem15ReloadWhitelistEv");
	ItemSystem = ServerModule.FindSymbol("_Z10ItemSystemv");
	GiveDefaultItems = ServerModule.FindSymbol("_ZN9CTFPlayer16GiveDefaultItemsEv");
	GetItemDefinition = ServerModule.FindSymbol("_ZN15CEconItemSchema17GetItemDefinitionEi");
	RemoveWearable = (void (*)(void *, void*))ServerModule.FindSymbol("_ZN11CBasePlayer14RemoveWearableEP13CEconWearable");
#else
	void *GetLoadoutItem = ServerModule.FindSignature(
				(unsigned char*)"\x55\x8B\xEC\x51\x53\x56\x8B\xF1\x8B\x0D\x00\x00\x00\x00\x57\x89\x75\xFC", "xxxxxxxxxx????xxxx");
	ReloadWhitelist = ServerModule.FindSignature(
				(unsigned char*)"\x55\x8B\xEC\x83\xEC\x0C\x53\x56\x57\x8B\xD9\xC6\x45\xFF\x01", "xxxxxxxxxxxxxxx");
	ItemSystem = ServerModule.FindSignature(
				(unsigned char*)"\xA1\x00\x00\x00\x00\x85\xC0\x75\x3F", "x????xxxx");
	RemoveWearable = (void (__thiscall*)(void *, void*))ServerModule.FindSignature(
				(unsigned char *)"\x55\x8B\xEC\x53\x8B\xD9\x57\x8B\xBB\x00\x00\x00\x00\x4F", "xxxxxxxxx????x");
	GiveDefaultItems = ServerModule.FindSignature(
				(unsigned char *)"\x55\x8B\xEC\x51\x53\x8B\xD9\x56\xFF\xB3", "xxxxxxxxxx");
	GetItemDefinition = ServerModule.FindSignature(
				(unsigned char*)"\x55\x8B\xEC\x56\x8B\xF1\x8D\x45\x08\x57\x50\x8D\x8E\x00\x00\x00\x00\xE8", "xxxxxxxxxxxxx????x");
#endif

	if(!GetLoadoutItem)
		Warning("Error Code 23\n");
	else
	{
#ifndef _LINUX
		PatchAddress((void*)GetLoadoutItem, 0xA7, 2, (unsigned char*)"\x90\x90");
#else
		PatchAddress((void*)GetLoadoutItem, 0x103, 6, (unsigned char*)"\x90\x90\x90\x90\x90\x90");
#endif
	}

	if(!ItemSystem)
		Warning("Error Code 24\n");
	if(!ReloadWhitelist)
		Warning("Error Code 25\n");
	else
	{
#ifndef _LINUX
		PatchAddress((void*)ReloadWhitelist, 0x45, 2, (unsigned char*)"\x90\x90");
#else
		PatchAddress((void*)ReloadWhitelist, 0x3E, 2, (unsigned char*)"\x90\x90");
#endif
	}

	if(!GetItemDefinition)
		Warning("Error Code 33\n");
	if(!RemoveWearable)
		Warning("Error code 15\n");
	if(!GiveDefaultItems)
		Warning("Error code 16\n");

	ConVarRef mp_tournament_whitelist("mp_tournament_whitelist");
	((ConVar*)mp_tournament_whitelist.GetLinkedConVar())->InstallChangeCallback(CItems::TournamentWhitelistCallback);

	return (GetLoadoutItem && ItemSystem && ReloadWhitelist && GetItemDefinition &&
			RemoveWearable && GiveDefaultItems);
}

void CItems::OnUnload()
{
	ConVarRef mp_tournament_whitelist("mp_tournament_whitelist");
	((ConVar*)mp_tournament_whitelist.GetLinkedConVar())->InstallChangeCallback(nullptr);
}

// pKItem can be an item or a prefab if doing recursion
char *CItems::GetAttributeValue(KeyValues *pKItem, const char *szAttribute)
{
	char *szValue = (char*)pKItem->GetString(szAttribute, NULL);

	if(!szValue)
	{
		KeyValues *pKPrefab = NULL;
		
		// We have a prefab
		if(pKItem->GetString("prefab", NULL))
		{
			char szPrefabName[65] = {};
			V_strncpy(szPrefabName, pKItem->GetString("prefab", NULL), sizeof(szPrefabName));

			char *pPrefab = strtok((char*)szPrefabName, " ");
			while(pPrefab != NULL)
			{
				pKPrefab = item_schema->FindKey("prefabs", true)->FindKey(pPrefab, false);
				if(pKPrefab)
				{
					szValue = (char*)pKPrefab->GetString(szAttribute, NULL);

					// If our prefab has a prefab, then call GetAttributeValue on the prefab
					const char *szPrefabPrefabName = pKPrefab->GetString("prefab", NULL);
					if(szPrefabPrefabName)
						szValue = GetAttributeValue(pKPrefab, szAttribute);

					// If we have found the attribute, get out of the loop
					if(szValue)
						break;
				}

				pPrefab = strtok(NULL, " ");
			}
		}
	}
	
	return szValue;
}

void CItems::RebuildWhitelist(IConVar *var, const char *pOldValue, float flOldValue)
{
	g_Items.item_whitelist->Clear();

	if(tftrue_whitelist_id.GetInt() != -1)
	{
		SOCKET sock = INVALID_SOCKET;
		if(ConnectToHost("whitelist.tf", sock))
		{
			int iWhiteListID = 0;
			char szConfigURL[50];
			char szConfigPath[50];

			// Handle int vs string whitelist ids
			if(sscanf(tftrue_whitelist_id.GetString(), "%d", &iWhiteListID) == 1)
			{
				V_snprintf(szConfigURL, sizeof(szConfigURL), "whitelist.tf/custom_whitelist_%d.txt", tftrue_whitelist_id.GetInt());
				V_snprintf(szConfigPath, sizeof(szConfigPath), "cfg/custom_whitelist_%d.txt", tftrue_whitelist_id.GetInt());
			}
			else
			{
				V_snprintf(szConfigURL, sizeof(szConfigURL), "whitelist.tf/%s.txt", tftrue_whitelist_id.GetString());
				V_snprintf(szConfigPath, sizeof(szConfigPath), "cfg/%s.txt", tftrue_whitelist_id.GetString());
			}

			// Download our whitelist
			g_Tournament.DownloadConfig(szConfigURL, sock);
			closesocket(sock);

			// Read the whitelist display name for the tftrue commands
			FileHandle_t fh = filesystem->Open(szConfigPath, "r", "MOD");
			if(fh)
			{
				g_Items.item_whitelist->LoadFromFile(filesystem, szConfigPath, "MOD");
				char szLine[255] = "";

				filesystem->ReadLine(szLine, sizeof(szLine), fh);
				filesystem->ReadLine(szLine, sizeof(szLine), fh);

				if(!strncmp("// Whitelist: ", szLine, 14))
				{
					V_strncpy(g_Items.szWhiteListName, szLine+13, sizeof(g_Items.szWhiteListName));
					g_Items.szWhiteListName[strlen(g_Items.szWhiteListName)-2] = '\0';
				}
				else
					V_strncpy(g_Items.szWhiteListName, "custom", sizeof(g_Items.szWhiteListName));

				filesystem->Close(fh);
			}
			else
				V_strncpy(g_Items.szWhiteListName, "error", sizeof(g_Items.szWhiteListName));
		}
	}
	else
	{
		if(strcmp(g_Items.szWhiteListChosen, ""))
			g_Items.item_whitelist->LoadFromFile(filesystem, g_Items.szWhiteListChosen, "MOD");
	}
	
	if(!g_Items.item_whitelist->FindKey("unlisted_items_default_to", false))
		g_Items.item_whitelist->SetInt("unlisted_items_default_to", 1);

	// Loop through all items
	for ( KeyValues *pKItem = g_Items.item_schema->FindKey("items", true)->GetFirstTrueSubKey(); pKItem; pKItem = pKItem->GetNextTrueSubKey() )
	{
		char *szItemSlot = g_Items.GetAttributeValue(pKItem, "item_slot");
		char *szItemName = g_Items.GetAttributeValue(pKItem, "name");
		char *szCraftClass = g_Items.GetAttributeValue(pKItem, "craft_class");
		char *szBaseItem = g_Items.GetAttributeValue(pKItem, "baseitem");
		
		// Make sure we have an item name and slot
		if(!szItemName || !szItemSlot)
			continue;
		
		// Do not try to add the item called "default"
		if(!strcmp(szItemName, "default"))
			continue;
		
		// Do not add base items
		if(szBaseItem && !strcmp(szBaseItem, "1"))
			continue;
		
		// Do not add craft tokens
		if(szCraftClass && !strcmp(szCraftClass, "craft_token"))
			continue;

		// Add item to the whitelist
		if((tftrue_no_hats.GetInt() && !strcmpi(szItemSlot, "head"))
				|| (tftrue_no_misc.GetInt() && !strcmpi(szItemSlot, "misc"))
				|| (tftrue_no_action.GetInt() && !strcmpi(szItemSlot, "action")))
			g_Items.item_whitelist->SetInt(szItemName, 0);
	}

	// Save the whitelist
	static ConVarRef mp_tournament_whitelist("mp_tournament_whitelist");
	g_Items.item_whitelist->SaveToFile(filesystem, "TFTrue_item_whitelist.txt", "MOD");
	mp_tournament_whitelist.SetValue("TFTrue_item_whitelist.txt");

	// Reload the whitelist
	void *pEconItemSystem = reinterpret_cast<ItemSystemFn>(g_Items.ItemSystem)();

	g_OldSpewOutputFunc = GetSpewOutputFunc();
	SpewOutputFunc(TFTrueSpew);
	reinterpret_cast<ReloadWhitelistFn>(g_Items.ReloadWhitelist)(pEconItemSystem);
	SpewOutputFunc(g_OldSpewOutputFunc);

	// Reload items of all players
	for ( int i = 1; i <= g_pServer->GetClientCount(); i++ )
	{
		CBasePlayer *pPlayer = (CBasePlayer*)CBaseEntity::Instance(i);
		if(!pPlayer)
			continue;

		if(engine->GetPlayerNetInfo(pPlayer->entindex())) // Not a bot
		{
			// Remove taunt
			*g_EntityProps.GetSendProp<int>(pPlayer, "m_Shared.m_nPlayerCond") &= ~TF2_PLAYERCOND_TAUNT;

			// Remove all wearables, GiveDefaultItems doesn't remove them for some reason
			CBaseEntity *pChild = *g_EntityProps.GetDataMapProp<EHANDLE>(pPlayer, "m_hMoveChild");
			while(pChild)
			{
				if(!strcmp(pChild->GetClassname(), "tf_wearable"))
				{
					g_Items.RemoveWearable(pPlayer, pChild);

					// Go back to the first child
					pChild = *g_EntityProps.GetDataMapProp<EHANDLE>(pPlayer, "m_hMoveChild");
				}
				else
					pChild = *g_EntityProps.GetDataMapProp<EHANDLE>(pChild, "m_hMovePeer");
			}

			// m_bRegenerating, set to true to keep ubercharge and other weapon data
			*(bool *)(g_EntityProps.GetSendProp<char>(pPlayer, "m_hItem")-1) = true;
			reinterpret_cast<GiveDefaultItemsFn>(g_Items.GiveDefaultItems)(pPlayer);
			*(bool *)(g_EntityProps.GetSendProp<char>(pPlayer, "m_hItem")-1) = false;
		}
	}
}
void CItems::TournamentWhitelistCallback(IConVar *var, const char *pOldValue, float flOldValue)
{
	static ConVarRef mp_tournament_whitelist("mp_tournament_whitelist");
	if(strcmp(mp_tournament_whitelist.GetString(), "TFTrue_item_whitelist.txt")) // Might be unneeded now?
	{
		V_strncpy(g_Items.szWhiteListChosen, mp_tournament_whitelist.GetString(), sizeof(g_Items.szWhiteListChosen));
		RebuildWhitelist(NULL, NULL, 0.0);
	}
}

const char* CItems::GetItemLogName(int iDefIndex)
{
	void *pEconItemSystem = reinterpret_cast<ItemSystemFn>(ItemSystem)();
	void *pItemDefinition = reinterpret_cast<GetItemDefinitionFn>(GetItemDefinition)((void*)((unsigned long)pEconItemSystem+4), iDefIndex);
	if(!pItemDefinition)
		return nullptr;

	return *(char**)((char*)pItemDefinition + 208);
}
