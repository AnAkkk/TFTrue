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

#include "irecipientfilter.h"

#include "valve_minmax_off.h"
#include <vector>

class MRecipientFilter : public IRecipientFilter
{
public:
	virtual bool IsReliable( void ) const;
	virtual bool IsInitMessage( void ) const;
	
	virtual int GetRecipientCount( void ) const;
	virtual int GetRecipientIndex( int slot ) const;
	void AddAllPlayers( void );
	void AddRecipient (int iPlayer );

	void MakeReliable( void );
	
private:
	bool m_bReliable = false;
	bool m_bInitMessage = false;
	std::vector< int > m_Recipients;
};
