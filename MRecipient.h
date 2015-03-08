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
