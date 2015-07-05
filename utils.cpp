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

#include "utils.h"

#include "AutoUpdater.h"
#include "items.h"
#include "logs.h"
#include "tournament.h"
#include "TFTrue.h"
#include "fov.h"
#include "bunnyhop.h"
#include "stats.h"
#include "sourcetv.h"
#include "MRecipient.h"

#ifdef WIN32
#include <WS2tcpip.h>
#endif

CSendProp g_SendProp;
CEntityProps g_EntityProps;

//-- Message things
int GetMessageType(const char * MessageName)
{
	char Name[255];
	int Size=0;
	for(int msg_type=0;1;msg_type++)
	{
		if(!gamedll->GetUserMessageInfo( msg_type, Name, 255, Size ))
			break;

		if(strcmp(Name,MessageName)==0)
			return msg_type;
	}
	return -1;
}

void ShowViewPortPanel( int iClientIndex, const char *name, bool bShow, KeyValues *data )
{
	MRecipientFilter filter;
	if(iClientIndex == -1)
		filter.AddAllPlayers();
	else
		filter.AddRecipient( iClientIndex );
	
	filter.MakeReliable();

	int count = 0;
	KeyValues *subkey = NULL;

	if ( data )
	{
		subkey = data->GetFirstSubKey();
		while ( subkey )
		{
			count++; subkey = subkey->GetNextKey();
		}

		subkey = data->GetFirstSubKey(); // reset
	}

	bf_write *pBuffer = engine->UserMessageBegin(&filter, GetMessageType("VGUIMenu"));
	pBuffer->WriteString(name);
	pBuffer->WriteByte( bShow?1:0 );
	pBuffer->WriteByte( count );

	// write additional data (be careful not more than 192 bytes!)
	while ( subkey )
	{
		pBuffer->WriteString( subkey->GetName() );
		pBuffer->WriteString( subkey->GetString() );
		subkey = subkey->GetNextKey();
	}
	
	engine->MessageEnd();
}

void ShowHTMLMOTD(int iClientIndex, const char *szTitle, const char *szURL)
{
	KeyValues *data = new KeyValues("data");
	data->SetString("title", szTitle);
	data->SetString("type", "2");
	data->SetString("msg", szURL);
	data->SetInt("cmd", 0);
	data->SetInt("customsvr", 1);
	data->SetInt("unload", 1);
	
	ShowViewPortPanel(iClientIndex, "info", true, data);
	
	data->deleteThis();
}

void TextMessage(int iClientIndex, const char *szMessage, ...)
{
	va_list vl;
	va_start(vl, szMessage);
	char szFormattedMessage[255];
	V_vsnprintf(szFormattedMessage, sizeof(szFormattedMessage), szMessage, vl);
	va_end(vl);

	MRecipientFilter filter;
	filter.MakeReliable();
	filter.AddRecipient( iClientIndex );
	bf_write *pBuffer = engine->UserMessageBegin( &filter, GetMessageType("TextMsg") );
	pBuffer->WriteByte( HUD_PRINTCENTER );
	pBuffer->WriteByte( true );
	pBuffer->WriteString( szFormattedMessage );
	engine->MessageEnd();
}

void Message(int iClientIndex, const char *szMessage, ...)
{
	va_list vl;
	va_start(vl, szMessage);
	char szFormattedMessage[255];
	V_vsnprintf(szFormattedMessage, sizeof(szFormattedMessage), szMessage, vl);
	va_end(vl);

	MRecipientFilter filter;
	filter.MakeReliable();
	filter.AddRecipient( iClientIndex );
	bf_write *pBuffer = engine->UserMessageBegin( &filter, GetMessageType("SayText") );
	pBuffer->WriteByte( iClientIndex );
	pBuffer->WriteString( szFormattedMessage );
	pBuffer->WriteByte( true );
	engine->MessageEnd();
}

// iClientIndex is only used to set the team color
void AllMessage(int iClientIndex, const char *szMessage, ...)
{
	char szBuffer[255];

	va_list vl;
	va_start(vl, szMessage);
	V_vsnprintf(szBuffer, sizeof(szBuffer), szMessage, vl);
	va_end(vl);

	MRecipientFilter filter;
	filter.MakeReliable();
	filter.AddAllPlayers();
	bf_write *pBuffer = engine->UserMessageBegin( &filter, GetMessageType("SayText") );

	// Ent index, used for setting the team color (\x03)
	pBuffer->WriteByte( iClientIndex );
	// Message
	pBuffer->WriteString( szBuffer );
	// True: raw text False: localized string
	pBuffer->WriteByte( true );

	engine->MessageEnd();
}

void AllMessage(const char *szMessage, ...)
{
	char szBuffer[255];

	va_list vl;
	va_start(vl, szMessage);
	V_vsnprintf(szBuffer, sizeof(szBuffer), szMessage, vl);
	va_end(vl);

	MRecipientFilter filter;
	filter.MakeReliable();
	filter.AddAllPlayers();
	bf_write *pBuffer = engine->UserMessageBegin( &filter, GetMessageType("SayText") );

	// Ent index, used for setting the team color (\x03)
	pBuffer->WriteByte( 0 );
	// Message
	pBuffer->WriteString( szBuffer );
	// True: raw text False: localized string
	pBuffer->WriteByte( true );

	engine->MessageEnd();
}

SendTable* CSendProp::GetSendTable( const char *szTableName )
{
	SendTable *pTable = NULL;
	for( ServerClass *pClass = gamedll->GetAllServerClasses(); pClass; pClass = pClass->m_pNext)
	{
		if (strcmp(pClass->m_pNetworkName, szTableName) == 0)
		{
			pTable = pClass->m_pTable;
			break;
		}
	}
	return pTable;
}


int CSendProp::GetSendPropOffset(SendTable *pTable, const char *szPropName, bool bShouldSplit)
{
	if(bShouldSplit)
	{
		char SendPropName[255];
		strncpy(SendPropName, szPropName, sizeof(SendPropName));
		szPropName = strtok(SendPropName, ".");
	}

	int offset = 0;
	int count = pTable->GetNumProps();

	SendProp *pProp;
	for (int i=0; i<count; i++)
	{
#undef GetPropA
		pProp = pTable->GetProp(i);

		if(szPropName)
		{
			if (strcmp(pProp->GetName(), szPropName) == 0)
			{
				offset = pProp->GetOffset();
				if(offset < 0) // vec_t offset can return negative offsets
					offset = -offset;
				szPropName = strtok(NULL,".");
			}
			if(szPropName)
			{
				if(pProp->GetDataTable())
				{
					offset += GetSendPropOffset(pProp->GetDataTable(),szPropName, false);
					if(offset)
					{
						break;
					}
				}
			}
			else
				break;
		}
		else
			break;
	}
	return offset;
}


SendProp* CSendProp::GetSendProp(SendTable *pTable, const char *szPropName, bool bShouldSplit)
{
	if(bShouldSplit)
	{
		char SendPropName[255];
		strncpy(SendPropName, szPropName, sizeof(SendPropName));
		szPropName = strtok(SendPropName, ".");
	}

	int count = pTable->GetNumProps();

	SendProp *pProp;
	for (int i=0; i<count; i++)
	{
#undef GetPropA
		pProp = pTable->GetProp(i);

		if(szPropName)
		{
			if (strcmp(pProp->GetName(), szPropName) == 0)
				szPropName = strtok(NULL, ".");
			if(szPropName)
			{
				if(pProp->GetDataTable())
				{
					pProp = GetSendProp(pProp->GetDataTable(),szPropName, false);
					if(pProp)
						return pProp;
				}
			}
			else
				return pProp;
		}
		else
			break;
	}
	return NULL;
}


/*SendProp* CSendProp::GetSubClassSendTable(SendTable *pTable, const char *szTableName)
{
	if(szTableName)
	{
		SendProp *pProp = pTable->GetProp(0);

		if(pProp)
		{
			if(!strcmp(pProp->GetName(), "baseclass"))
			{
				if(pProp->GetDataTable())
				{
					pProp = GetSubClassSendTable(pProp->GetDataTable(), szTableName);
					if(pProp)
					{
						if(!strcmp(pProp->GetDataTable()->GetName(), szTableName))
							return pProp;
					}
				}
			}
			return pProp;
		}
	}
	return NULL;
}*/

/*
void DumpProps(int num, SendTable *pTable)
{
	int count = pTable->GetNumProps();
	SendProp *pProp;
	for (int i=0; i<count; i++)
	{
		pProp = pTable->GetProp(i);
		for (int j = 0; j < num; j++)
			Msg("----");
		if(!strcmp(pProp->GetName(), "baseclass"))
			Msg("Sub class: %s\n", pTable->GetName());
		else
			Msg("Prop Name:%s Offset: %d\n",pProp->GetName(),pProp->GetOffset());
		if(pProp->GetDataTable())
		{
			DumpProps(num + 1, pProp->GetDataTable());
		}
	}
}

CON_COMMAND(dumpnetworkvars,"")
{
	ServerClass *pClass = gamedll->GetAllServerClasses();

	while (pClass)
	{
		Msg("Class Name:%s\n",pClass->m_pNetworkName);
		DumpProps(0,pClass->m_pTable);

		pClass = pClass->m_pNext;
	}
}

void DumpDataMap2(int num, datamap_t *pMap)
{

	int count = pMap->dataNumFields;
	SendProp *pProp;
	for (int i=0; i<count; i++)
	{
		if (pMap->dataDesc[i].fieldName == NULL)
		{
			continue;
		}

		for (int j = 0; j < num; j++)
			Msg("----");

		Msg("Data Name:%s Offset: %d\n",pMap->dataDesc[i].fieldName,pMap->dataDesc[i].fieldOffset[TD_OFFSET_NORMAL]);
		if(pMap->dataDesc[i].td)
		{
			DumpDataMap2(num + 1, pMap->dataDesc[i].td);
		}
	}
}

void DumpDataMap1(datamap_t *pMap)
{
	while(pMap)
	{
		Msg("Class Name:%s\n",pMap->dataClassName);
		DumpDataMap2(0,pMap);
		pMap = pMap->baseMap;
	}
}*/

// FIXME: doesn't properly support searching for a child
int UTIL_FindInDataMap(datamap_t *pMap, const char *name)
{
	while (pMap)
	{
		for (int i=0; i<pMap->dataNumFields; i++)
		{
			if (pMap->dataDesc[i].fieldName == NULL)
				continue;
			if (strcmp(name, pMap->dataDesc[i].fieldName) == 0)
				return pMap->dataDesc[i].fieldOffset[TD_OFFSET_NORMAL];
			if (pMap->dataDesc[i].td)
			{
				int offset;
				if ((offset=UTIL_FindInDataMap(pMap->dataDesc[i].td, name)) != -1)
					return offset;
			}
		}
		pMap = pMap->baseMap;
	}

	return -1;
}

int GetEntIndexFromUserID(int iUserID)
{
	for(int i = 0; i < g_pServer->GetClientCount(); i++)
	{
		IClient* pClient = g_pServer->GetClient(i);
		if(pClient && pClient->GetUserID() == iUserID)
			return i + 1;
	}

	return -1;
}

SpewOutputFunc_t g_OldSpewOutputFunc;

SpewRetval_t TFTrueSpew( SpewType_t spewType, const tchar *pMsg )
{
	if(strstr(pMsg, "-> Allowing") || strstr(pMsg, "-> Removing") || strstr(pMsg, "-> Could not find an item definition named"))
		return SPEW_CONTINUE;

	return g_OldSpewOutputFunc(spewType, pMsg);
}

// Problem: the memory is never freed, find a solution for that someday
unsigned char *PatchAddress(const void *pAddress, unsigned int iOffset, unsigned int iNumBytes, unsigned char* strBytes)
{   
	unsigned char* ucSavedBytes = new unsigned char[iNumBytes];
#ifdef _WIN32
	DWORD dwback;
	VirtualProtect(((unsigned char*)pAddress + iOffset), iNumBytes, PAGE_READWRITE, &dwback);
#else
	mprotect((void*)((unsigned int)((unsigned long)pAddress + iOffset) & ~(sysconf(_SC_PAGE_SIZE) - 1)), iNumBytes + ((unsigned int)((unsigned long)pAddress + iOffset) & (sysconf(_SC_PAGE_SIZE) - 1)), PROT_READ | PROT_WRITE | PROT_EXEC);
#endif

	for(unsigned int i = 0; i < iNumBytes; i++)
	{
		ucSavedBytes[i] = ((unsigned char*)(pAddress))[iOffset+i];
		((unsigned char*)(pAddress))[iOffset+i] = strBytes[i];
	}
#ifdef _WIN32
	DWORD dwback2;
	VirtualProtect(((unsigned char*)pAddress + iOffset), iNumBytes, dwback, &dwback2);		// Protect again the source address
#else
	mprotect((void*)((unsigned int)((unsigned long)pAddress + iOffset) & ~(sysconf(_SC_PAGE_SIZE) - 1)), iNumBytes + ((unsigned int)((unsigned long)pAddress + iOffset) & (sysconf(_SC_PAGE_SIZE) - 1)), PROT_READ | PROT_EXEC);
#endif   
	return ucSavedBytes;
}

void PrintTFTrueInfos(edict_t *pEntity)
{
	char Line[255];
	sprintf(Line,"TFTrue Version %s\n", tftrue_version.GetString() );
	engine->ClientPrintf(pEntity,Line);
	sprintf(Line,"Freeze Cam: %s\n",(tftrue_freezecam.GetBool() == true ) ? "On":"Off");
	engine->ClientPrintf(pEntity,Line);
	sprintf(Line,"Items: Hats: %s | Misc: %s | Action: %s",
			(tftrue_no_hats.GetBool() == true ) ? "Off":"On",
			(tftrue_no_misc.GetBool() == true ) ? "Off":"On",
			(tftrue_no_action.GetBool() == true ) ? "Off":"On");

	int iItemLineLength = strlen(Line);

	if(tftrue_whitelist_id.GetInt() != -1)
	{
		if(!strcmp(g_Items.GetWhitelistName(), "custom"))
			sprintf(Line+iItemLineLength, " | Custom Whitelist ID: %d\n", tftrue_whitelist_id.GetInt());
		else
			sprintf(Line+iItemLineLength, " | Whitelist:%s\n", g_Items.GetWhitelistName());
	}
	else
		sprintf(Line+iItemLineLength, " | Whitelist: None\n");

	engine->ClientPrintf(pEntity,Line);

	sprintf(Line,"Maximum FoV allowed: %d\n",(tftrue_maxfov.GetInt()));
	engine->ClientPrintf(pEntity,Line);
	sprintf(Line,"Bunny Hop: %s\n",(tftrue_bunnyhop.GetBool() == true) ? "On":"Off");
	engine->ClientPrintf(pEntity,Line);
	sprintf(Line,"Restore stats: %s\n",(tftrue_restorestats.GetBool() == true) ? "On":"Off");
	engine->ClientPrintf(pEntity,Line);

	static ConVarRef mp_tournament("mp_tournament");
	static ConVarRef tf_gamemode_mvm("tf_gamemode_mvm");

	if(mp_tournament.GetBool() && !tf_gamemode_mvm.GetBool())
	{
		sprintf(Line,"STV Autorecord: %s\n",(tftrue_tv_autorecord.GetBool() == true ) ? "On":"Off");
		engine->ClientPrintf(pEntity,Line);
		
		switch(tftrue_tournament_config.GetInt())
		{
		case CTournament::CONFIG_NONE:
			sprintf(Line, "Tournament config: \005None");
			engine->ClientPrintf(pEntity, Line);
			break;
		case CTournament::CONFIG_ETF2L6v6:
			sprintf(Line, "Tournament config: \005ETF2L 6v6");
			engine->ClientPrintf(pEntity, Line);
			break;
		case CTournament::CONFIG_ETF2L9v9:
			sprintf(Line, "Tournament config: \005ETF2L 9v9");
			engine->ClientPrintf(pEntity, Line);
			break;
		}
	}
}

IChangeInfoAccessor *CBaseEdict::GetChangeAccessor()
{
	return engine->GetChangeAccessor((const edict_t*)this);
}

bool ConnectToHost(const char *szHostName, SOCKET &retsock)
{
	SOCKET sock = INVALID_SOCKET;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock == INVALID_SOCKET)
	{
		Msg("[TFTrue] Failed to create socket.\n");
		engine->LogPrint("[TFTrue] Failed to create socket.\n");
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
		return false;
	}

	retsock = sock;

	return true;
}

char GetBase64Value(char c)
{
	if(c >= 'A' && c <= 'Z')
		c -= 'A';
	else if(c >= 'a' && c <= 'z')
		c = c-'a'+26;
	else if(c >= '0' && c <= '9')
		c = c-'0'+52;
	else if(c == '+')
		c = 62;
	else if (c == '/')
		c = 63;
	else if (c == '=')
		c = -1;

	return c;
}

// From stackoverflow.com/questions/12232093
#ifdef _WIN32
#define NTSTATUS ULONG
#define STATUS_SUCCESS 0x00000000
typedef struct _PEB_LDR_DATA {
	ULONG Length;
	BOOLEAN Initialized;
	PVOID SsHandle;
	LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;
} PEB_LDR_DATA, *PPEB_LDR_DATA;


typedef struct _LSA_UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR  Buffer;
} LSA_UNICODE_STRING, *PLSA_UNICODE_STRING;
typedef LSA_UNICODE_STRING UNICODE_STRING, *PUNICODE_STRING;

typedef struct _RTL_USER_PROCESS_PARAMETERS {
	BYTE           Reserved1[16];
	PVOID          Reserved2[10];
	UNICODE_STRING ImagePathName;
	UNICODE_STRING CommandLine;
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

typedef VOID (NTAPI *PPS_POST_PROCESS_INIT_ROUTINE) (VOID);

typedef struct _PEB {
	BYTE                          Reserved1[2];
	BYTE                          BeingDebugged;
	BYTE                          Reserved2[1];
	PVOID                         Reserved3[2];
	PPEB_LDR_DATA                 Ldr;
	PRTL_USER_PROCESS_PARAMETERS  ProcessParameters;
	BYTE                          Reserved4[104];
	PVOID                         Reserved5[52];
	PPS_POST_PROCESS_INIT_ROUTINE PostProcessInitRoutine;
	BYTE                          Reserved6[128];
	PVOID                         Reserved7[1];
	ULONG                         SessionId;
} PEB, *PPEB;
typedef struct _PROCESS_BASIC_INFORMATION
{
	PVOID Reserved1;
	PPEB PebBaseAddress;
	PVOID Reserved2[2];
	ULONG_PTR UniqueProcessId;
	PVOID Reserved3;
} PROCESS_BASIC_INFORMATION;
typedef enum _PROCESSINFOCLASS
{
	ProcessBasicInformation = 0,
} PROCESSINFOCLASS, *PPROCESSINFOCLASS;

typedef struct _LDRP_CSLIST
{
	PSINGLE_LIST_ENTRY Tail;
} LDRP_CSLIST, *PLDRP_CSLIST;
typedef enum _LDR_DDAG_STATE
{
	LdrModulesMerged = -5,
	LdrModulesInitError = -4,
	LdrModulesSnapError = -3,
	LdrModulesUnloaded = -2,
	LdrModulesUnloading = -1,
	LdrModulesPlaceHolder = 0,
	LdrModulesMapping = 1,
	LdrModulesMapped = 2,
	LdrModulesWaitingForDependencies = 3,
	LdrModulesSnapping = 4,
	LdrModulesSnapped = 5,
	LdrModulesCondensed = 6,
	LdrModulesReadyToInit = 7,
	LdrModulesInitializing = 8,
	LdrModulesReadyToRun = 9
} LDR_DDAG_STATE;
typedef struct _LDR_DDAG_NODE
{
	LIST_ENTRY Modules;
	PVOID ServiceTagList;
	ULONG LoadCount;//this is where its located in windows 8
	ULONG ReferenceCount;
	ULONG DependencyCount;
	union
	{
		LDRP_CSLIST Dependencies;
		SINGLE_LIST_ENTRY RemovalLink;
	};
	LDRP_CSLIST IncomingDependencies;
	LDR_DDAG_STATE State;
	SINGLE_LIST_ENTRY CondenseLink;
	ULONG PreorderNumber;
	ULONG LowestLink;
} LDR_DDAG_NODE, *PLDR_DDAG_NODE;
typedef struct _LDR_MODULE {
	LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;
	PVOID BaseAddress;
	PVOID EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	ULONG Flags;
	USHORT obsoleteLoadCount;//in windows 8 this is obsolete
	USHORT TlsIndex;//but we can still use it in win 7 and below
	union
	{
		LIST_ENTRY HashLinks;
		struct CheckPtr
		{
			PVOID SectionPointer;
			ULONG CheckSum;
		};
	};
	union
	{
		ULONG TimeDateStamp;
		PVOID LoadedImports;
	};
	struct _ACTIVATION_CONTEXT *EntryPointActivationContext;
	PVOID PatchInformation;
	PLDR_DDAG_NODE DdagNode;
} LDR_MODULE, *PLDR_MODULE;


typedef NTSTATUS (__stdcall *pfnZwQueryInformationProcess) (HANDLE, PROCESSINFOCLASS,
															PVOID, ULONG, PULONG);
pfnZwQueryInformationProcess ZwQueryInformationProcess;

DWORD GetModuleLoadCount(HMODULE hmod)
{
	HMODULE hModule = LoadLibrary("ntdll.dll");
	if(hModule==NULL)
		return NULL;
	ZwQueryInformationProcess = (pfnZwQueryInformationProcess) GetProcAddress(hModule,
																			  "ZwQueryInformationProcess");
	if (ZwQueryInformationProcess == NULL) {
		FreeLibrary(hModule);
		return NULL;    // failed to get PEB
	}

	PROCESS_BASIC_INFORMATION pbi;
	PROCESSINFOCLASS pic = ProcessBasicInformation;
	if (ZwQueryInformationProcess(GetCurrentProcess(), pic, &pbi, sizeof(pbi), NULL) != STATUS_SUCCESS)
	{
		// ZwQueryInformationProcess failed...
		FreeLibrary(hModule);
		return NULL;
	}
	FreeLibrary(hModule);

	LDR_MODULE *peb_ldr_module = (LDR_MODULE*)pbi.PebBaseAddress->Ldr->InLoadOrderModuleList.Flink;
	while((peb_ldr_module = (LDR_MODULE*)peb_ldr_module->InLoadOrderModuleList.Flink)
		  !=(LDR_MODULE*)pbi.PebBaseAddress->Ldr->InLoadOrderModuleList.Blink)
	{
		if(peb_ldr_module->BaseAddress == hmod)
		{
			//well this actualy works in windows 8...
			//and probably vista with aero enabled as well...
			//anyway if it is obsolete its always 6
			//so we can if it out like this...
			if(peb_ldr_module->obsoleteLoadCount == 6)
				return peb_ldr_module->DdagNode->LoadCount;
			else
				return peb_ldr_module->obsoleteLoadCount;
		}
	}
	if(peb_ldr_module->BaseAddress==hmod)
	{
		if(peb_ldr_module->obsoleteLoadCount == 6)
			return peb_ldr_module->DdagNode->LoadCount;
		else
			return peb_ldr_module->obsoleteLoadCount;
	}
	return NULL;
}
#endif
