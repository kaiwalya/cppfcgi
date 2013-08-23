//
//  sock.h
//  fcgi
//
//  Created by Kaiwalya Kher on 8/17/13.
//  Copyright (c) 2013 Kaiwalya Kher. All rights reserved.
//

#ifndef fcgi_sock_h
#define fcgi_sock_h


#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "komm/global.h"

namespace komm
{
	namespace sock
	{
		namespace bsd
		{
			enum class NetworkProtocol
			{
				inet,
			};
			
			int NetworkProtocolToCInt(const NetworkProtocol & d) 
			{
				switch (d) {
					case NetworkProtocol::inet:
						return AF_INET;
				}
			}
			
			NetworkProtocol CIntToNetworkProtocol(int domain)
			{
				switch (domain) {
					case AF_INET:
						return NetworkProtocol::inet;
					default:
						throw std::domain_error(globalstrings()[StringID::Socket_Socket_BadDomain]);
				}
			}
			
			
			enum class TransportSemantics
			{
				datagram,
				stream,
			};
			
			int TransportSemanticsToCInt(const TransportSemantics & s)
			{
				switch (s) {
					case TransportSemantics::stream:
						return SOCK_STREAM;
					case TransportSemantics::datagram:
						return SOCK_DGRAM;
				}
			}
			
			TransportSemantics CIntToTransportSemantics(int semantics)
			{
				switch (semantics) {
					case SOCK_DGRAM:
						return TransportSemantics::datagram;
					case SOCK_STREAM:
						return TransportSemantics::stream;
					default:
						throw std::domain_error(globalstrings()[StringID::Socket_Socket_BadSemantics]);
				}
			}
			
			enum class TransportProtocol
			{
				tcp,
				udp,
			};
			
			int TransportProtocolToCInt(const TransportProtocol & p)
			{
				int protocol;
				switch (p) {
					case TransportProtocol::tcp:
						protocol = IPPROTO_TCP;
						break;
					case TransportProtocol::udp:
						protocol = IPPROTO_UDP;
						break;
				}
				return protocol;
			}
			
			TransportProtocol CIntToTransportProtocol(int protocol)
			{
				switch (protocol) {
					case IPPROTO_TCP:
						return TransportProtocol::tcp;
					case IPPROTO_UDP:
						return TransportProtocol::udp;
					default:
						throw std::domain_error(globalstrings()[StringID::Socket_Socket_BadProtocol]);
				}
			}
			
		}
	}
}

#endif
