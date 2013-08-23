//
//  global.h
//  fcgi
//
//  Created by Kaiwalya Kher on 8/17/13.
//  Copyright (c) 2013 Kaiwalya Kher. All rights reserved.
//

#ifndef fcgi_global_h
#define fcgi_global_h

#include <string>
#include <map>
#include "komm/logic.h"

namespace komm
{
	enum class StringID
	{
		Socket_Socket_BadDomain,
		Socket_Socket_BadSemantics,
		Socket_Socket_BadProtocol,
		Socket_Socket_CannotOpen,
		Socket_Socket_CannotBind,
		Socket_Socket_CannotListen,
		Socket_Socket_CannotAccept,
		Socket_Socket_Invalid,
		
	};
	
	class Strings
	{
	private:
		std::map<StringID, std::string>  m_strings;
	public:
		Strings();
		
		Strings(const Strings &) = delete;
		Strings(Strings &&) = delete;
		Strings & operator = (const Strings &) = delete;
		Strings & operator = (Strings &&) = delete;
		const std::string & operator[](StringID id) const
		{
			return m_strings.at(id);
		}
		~Strings(){}
	};
	
	class G
	{
		Strings s;
	public:
		const Strings & strings() const
		{
			return s;
		}
	};
	
	using Global = logic::singleton<G>;
	
	G & global()
	{
		return Global::instance();
	}
	
	const Strings & globalstrings()
	{
		return global().strings();
	}
	
}

#endif
