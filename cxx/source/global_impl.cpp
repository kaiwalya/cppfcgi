//
//  global_impl.cpp
//  fcgi
//
//  Created by Kaiwalya Kher on 8/17/13.
//  Copyright (c) 2013 Kaiwalya Kher. All rights reserved.
//

#include "global_impl.h"

komm::Strings::Strings()
{
	m_strings.emplace(StringID::Socket_Socket_BadDomain, "Bad Domain Specified to initialize Socket");
	m_strings.emplace(StringID::Socket_Socket_BadSemantics, "Bad Semantic Specified to initialize Socket");
	m_strings.emplace(StringID::Socket_Socket_BadProtocol, "Bad Protocol Specified to initialize Socket");
	m_strings.emplace(StringID::Socket_Socket_CannotOpen, "Cannot open socket with the specified parameters");
	m_strings.emplace(StringID::Socket_Socket_CannotBind, "Cannot bind socket with the specified address");
	m_strings.emplace(StringID::Socket_Socket_CannotListen, "Cannot listen socket");
	m_strings.emplace(StringID::Socket_Socket_CannotAccept, "Cannot accept client socket");
	m_strings.emplace(StringID::Socket_Socket_Invalid, "The socket you are trying to read is invalid");
}