//
//  sock_impl.h
//  fcgi
//
//  Created by Kaiwalya Kher on 8/17/13.
//  Copyright (c) 2013 Kaiwalya Kher. All rights reserved.
//

#ifndef __fcgi__sock_impl__
#define __fcgi__sock_impl__

#include "komm/sock.h"

namespace komm
{
	namespace sock
	{
	/*
		class Address
		{
		private:
			Domain m_domain;
			std::unique_ptr<unsigned char []> m_address;
			
		protected:
			sockaddr & address() {return *(sockaddr *)m_address.get();}
			
		public:
			Address(Domain d, size_t addressSize): m_domain(d), m_address(new unsigned char[addressSize])
			{
				address().sa_len = addressSize;
				switch (m_domain) {
					case Domain::inet:
						address().sa_family = AF_INET;
						break;
				}
			}
			const sockaddr & address() const { return *(sockaddr *)m_address.get(); }
			const size_t size() const { return address().sa_len; }
			const Domain & domain() const { return m_domain; }
		};
		
		class INetAddress: public Address
		{
			static const INetAddress s_any;
			static const size_t s_szAddress = sizeof(sockaddr_in);
		public:
			static const INetAddress & any() {return s_any;}
			
		private:
			void copyOnSelf(const INetAddress & src)
			{
				memcpy(&address(), &src.address(), src.size());
			}
			
			sockaddr_in & inaddress()
			{
				return *(sockaddr_in *)&address();
			}
		protected:
			size_t getSize() const
			{
				return sizeof(sockaddr_in);
			}
			
		public:
			enum class NamedAddressType
			{
				any,
			};
			
			INetAddress(NamedAddressType type):Address(Domain::inet, s_szAddress)
			{
				switch (type) {
					case NamedAddressType::any:
						sockaddr_in & addr_in = inaddress();
						addr_in.sin_addr.s_addr = htonl(INADDR_ANY);
						break;
				}
			}
			
			INetAddress():Address(Domain::inet, s_szAddress)
			{
				copyOnSelf(any());
			}
			
			INetAddress(const INetAddress & other): Address(Domain::inet, s_szAddress)
			{
				copyOnSelf(other);
			}
			
			void setPort(short port)
			{
				inaddress().sin_port = htons(port);
			}
		};
		
		class Socket
		{
			
			int m_socket;
		protected:
			Socket()
			{
				m_socket = -1;
			}
			
			Socket(const Socket &) = delete;
			Socket(Socket &&) = delete;
			Socket & operator = (Socket &) = delete;
			Socket & operator = (Socket &&) = delete;
			
			void setSocket(int socket)
			{
				m_socket = socket;
			}
			
		public:
			int getSocket() const
			{
				if (m_socket < 0)
					throw std::domain_error(globalstrings()[StringID::Socket_Socket_Invalid]);
				return m_socket;
			}
			
			bool isValid() {return m_socket > 0;}
			
			~Socket()
			{
				if (isValid())
					close(m_socket);
			}
		};
		
		class ServerSocket: public Socket
		{
		public:
			ServerSocket() = delete;
			ServerSocket(const ServerSocket &) = delete;
			ServerSocket(ServerSocket &&) = delete;
			ServerSocket & operator = (ServerSocket &) = delete;
			ServerSocket & operator = (ServerSocket &&) = delete;
			
			ServerSocket(const Address & addr, Semantics s, Protocol p)
			{
				int domain = DomainToCInt(addr.domain());
				int semantics = SemanticsToCInt(s);
				int protocol = ProtocolToCInt(p);
				
				int temp;
				temp = socket(domain, semantics, protocol);
				if (temp < 0)
					throw std::domain_error(globalstrings()[StringID::Socket_Socket_CannotOpen]);
				setSocket(temp);
				
				temp = bind(getSocket(), &addr.address(), (socklen_t)addr.size());
				if (temp < 0)
					throw std::domain_error(globalstrings()[StringID::Socket_Socket_CannotBind]);
				
				temp = listen(getSocket(), 32);
				if (temp < 0)
					throw std::domain_error(globalstrings()[StringID::Socket_Socket_CannotListen]);
				
			}
			
			
		};
	 */
	}
}
#endif /* defined(__fcgi__sock_impl__) */
