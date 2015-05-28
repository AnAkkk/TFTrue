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

#include "SDK.h"

#include "MRecipient.h"

#include "valve_minmax_off.h"
#include <algorithm>

int MRecipientFilter::GetRecipientCount() const
{
	return m_Recipients.size();
}

int MRecipientFilter::GetRecipientIndex(int slot) const
{
	if ( slot < 0 || slot >= GetRecipientCount() )
		return -1;

	return m_Recipients[ slot ];
}

bool MRecipientFilter::IsInitMessage() const
{
	return m_bInitMessage;
}

bool MRecipientFilter::IsReliable() const
{
	return m_bReliable;
}

void MRecipientFilter::MakeReliable( void )
{
	m_bReliable = true;
}

void MRecipientFilter::AddAllPlayers()
{
	m_Recipients.clear();
	for ( int i = 0; i < g_pServer->GetNumClients(); i++ )
	{
		IClient* pClient = g_pServer->GetClient(i);
		if ( !pClient || !pClient->IsConnected())
			continue;

		AddRecipient(i + 1);
	}
} 
void MRecipientFilter::AddRecipient( int iPlayer )
{
	// Already in list
	if ( std::find(m_Recipients.begin(), m_Recipients.end(), iPlayer ) != m_Recipients.end() )
		return;

	m_Recipients.insert(m_Recipients.end(), iPlayer );
}
