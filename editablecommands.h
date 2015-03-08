#pragma once

#include "SDK.h"


class EditableConCommandBase
{
public:
	virtual						~EditableConCommandBase( void );
	ConCommandBase				*m_pNext;
	bool						m_bRegistered;
	const char 					*m_pszName;
	const char 					*m_pszHelpString;
	int							m_nFlags;
	static ConCommandBase		*s_pConCommandBases;
	static IConCommandBaseAccessor	*s_pAccessor;
};

class EditableConCommand : public EditableConCommandBase
{
public:
	virtual ~EditableConCommand( void );
	union
	{
		FnCommandCallbackVoid_t m_fnCommandCallbackV1;
		FnCommandCallback_t m_fnCommandCallback;
		ICommandCallback *m_pCommandCallback;
	};

	union
	{
		FnCommandCompletionCallback	m_fnCompletionCallback;
		ICommandCompletionCallback *m_pCommandCompletionCallback;
	};

	bool m_bHasCompletionCallback : 1;
	bool m_bUsingNewCommandCallback : 1;
	bool m_bUsingCommandCallbackInterface : 1;
};

class EditableConVar: public EditableConCommandBase, public IConVar
{
public:
	virtual						~EditableConVar( void );
	ConVar						*m_pParent;
	const char					*m_pszDefaultValue;
	char						*m_pszString;
	int							m_StringLength;
	float						m_fValue;
	int							m_nValue;
	bool						m_bHasMin;
	float						m_fMinVal;
	bool						m_bHasMax;
	float						m_fMaxVal;
	FnChangeCallback_t			m_fnChangeCallback;
};
