#include <stdexcept>
#include <map>
#include <string>
#include <memory>
#include <thread>
#include <vector>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>


namespace fcgi
{
	namespace logic
	{
		template<typename T>
		class singleton
		{
		private:
			static T * m_globalObject;
		public:
			static T & instance()
			{
				if (!m_globalObject)
					m_globalObject = new T();
				return *m_globalObject;
			}
			
			static void destroy()
			{
				delete m_globalObject;
				m_globalObject = nullptr;
			}
		};
		
	}
	
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
		Strings()
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
	
	
	enum class Domain
	{
		inet,
	};
	
	int DomainToCInt(const Domain & d)
	{
		switch (d) {
			case Domain::inet:
				return AF_INET;
		}
	}
	
	Domain CIntToDomain(int domain)
	{
		switch (domain) {
			case AF_INET:
				return Domain::inet;
			default:
				throw std::domain_error(globalstrings()[StringID::Socket_Socket_BadDomain]);
		}
	}
	
	
	enum class Semantics
	{
		datagram,
		stream,
	};
	
	int SemanticsToCInt(const Semantics & s)
	{
		switch (s) {
			case Semantics::stream:
				return SOCK_STREAM;
			case Semantics::datagram:
				return SOCK_DGRAM;
		}
	}
	
	Semantics CIntToSemantics(int semantics)
	{
		switch (semantics) {
			case SOCK_DGRAM:
				return Semantics::datagram;
			case SOCK_STREAM:
				return Semantics::stream;
			default:
				throw std::domain_error(globalstrings()[StringID::Socket_Socket_BadSemantics]);
		}
	}
	
	enum class Protocol
	{
		tcp,
		udp,
	};
	
	int ProtocolToCInt(const Protocol & p)
	{
		int protocol;
		switch (p) {
			case Protocol::tcp:
				protocol = IPPROTO_TCP;
				break;
			case Protocol::udp:
				protocol = IPPROTO_UDP;
				break;
		}
		return protocol;
	}
	
	Protocol CIntToProtocol(int protocol)
	{
		switch (protocol) {
			case IPPROTO_TCP:
				return Protocol::tcp;
			case IPPROTO_UDP:
				return Protocol::udp;
			default:
				throw std::domain_error(globalstrings()[StringID::Socket_Socket_BadProtocol]);
		}
	}

	
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
}

#include "cppa/cppa.hpp"
#include <iostream>
#include <sstream>

namespace fcgi
{
	enum class RecordType: uint8_t
	{
		begin = 1,
		abort,
		end,
		params,
		stdIn,
		stdOut,
		stdErr,
		data,
		getValues,
		getValuesResult,
		unknown,
		max = unknown
	};
	
	struct context
	{
		cppa::actor_ptr stdout;
	};
	
	struct RecordHeader
	{
		uint8_t version;
		uint8_t type;
		uint16_t requestID;
		uint16_t length;
		uint8_t padding;
		uint8_t unused;
	};
	
	struct Record
	{
		RecordHeader header;
		std::shared_ptr<uint8_t> data;
	};
	
	enum class Role: uint16_t
	{
		reponder = 1,
		authorizer,
		filter,
	};
	
	struct BeginRequestRecordData
	{
		uint16_t role;
		uint8_t flags;
		uint8_t unused[5];
	};
	
	void swapBytes(uint64_t & data)
	{
		uint8_t (&d)[8] = *(uint8_t (*)[8])&data;
		std::swap(d[0], d[7]);
		std::swap(d[1], d[6]);
		std::swap(d[2], d[5]);
		std::swap(d[3], d[4]);
	}
	
	void swapBytes(uint32_t & data)
	{
		uint8_t (&d)[4] = *(uint8_t (*)[4])&data;
		std::swap(d[0], d[3]);
		std::swap(d[1], d[2]);
	}
	
	void swapBytes(uint16_t & data)
	{
		uint8_t (&d)[2] = *(uint8_t (*)[2])&data;
		std::swap(d[0], d[1]);
	}
	
	void nh(RecordHeader & rh)
	{
		swapBytes(rh.requestID);
		swapBytes(rh.length);
	};
	
	void nh(BeginRequestRecordData & data)
	{
		swapBytes(data.role);
	}
	
	void parseLength(const uint8_t * & ptr, uint32_t & length){
		if (*ptr >> 7 == 0)
		{
			length = *ptr;
			ptr++;
		}
		else
		{
			length = *(uint32_t *)ptr;
			swapBytes(length);
			ptr+=4;
		}
	};
	
	class ConnectedSocket
	{
		int m_socket;
	public:
		ConnectedSocket(int iSocket): m_socket(iSocket){}
		~ConnectedSocket()
		{
			shutdown(m_socket, SHUT_RDWR);
			close(m_socket);
		}
	};
	
	cppa::actor_ptr newRequestHandler()
	{
		return cppa::spawn([](){
			cppa::become(cppa::on_arg_match >> [] (int, Record & rec) {
				switch (static_cast<RecordType>(rec.header.type)) {
					case RecordType::begin:
					{
						BeginRequestRecordData & beginReqest = *(BeginRequestRecordData *)rec.data.get();
						nh(beginReqest);
						Role & r = (Role &)beginReqest.role;
						switch (r) {
							case Role::authorizer:
								break;
							case Role::filter:
								break;
							case Role::reponder:
								break;
								
							default:
								break;
						}
						break;
					}
						
					case RecordType::abort:
						break;
						
					case RecordType::end:
						break;
						
					case RecordType::params:
					{
						const uint8_t * ptrStart = rec.data.get();
						const uint8_t * ptrCurrent = rec.data.get();
						uint32_t nameLength;
						uint32_t valueLength;
						ptrdiff_t usedLength = 0; //ptrCurrent - ptrStart;
						while(usedLength < rec.header.length)
						{
							parseLength(ptrCurrent, nameLength);
							parseLength(ptrCurrent, valueLength);
							std::string name((const char *)ptrCurrent, nameLength); ptrCurrent += nameLength;
							std::string value((const char *)ptrCurrent, valueLength); ptrCurrent += valueLength;
							cppa::aout << name << ":" << value << std::endl;
							usedLength = ptrCurrent - ptrStart;
						}
						break;
					}
						
					case RecordType::stdIn:
					{
						//const uint8_t * ptrStart = rec.data.get();
						//const uint8_t * ptrCurrent = rec.data.get();
						break;
					}
					case RecordType::stdOut:
						break;
					case RecordType::stdErr:
						break;
					case RecordType::data:
						break;
					case RecordType::getValues:
						break;
					case RecordType::getValuesResult:
						break;
					default:
						break;
				}
			});
		});
	}
	
	void newConnection(context & c, cppa::cow_tuple<int, sockaddr, socklen_t> sockInfo)
	{
		cppa::spawn<cppa::blocking_api>([&c] () {
			cppa::receive(cppa::on_arg_match >> [&c] (int & iSocket, sockaddr & addr, socklen_t & len) {
				ConnectedSocket s(iSocket);
				send(c.stdout, "New Socket: ", iSocket);
				
				std::map<uint16_t, cppa::actor_ptr> handlers;
				bool exit = false;
				while(!exit)
				{
					ssize_t bytesRead = 0;
					cppa::cow_tuple<int, Record> recordTuple;
					Record & record = cppa::get_ref<1>(recordTuple);
					RecordHeader & header = record.header;
					while(1)
					{
						bytesRead = read(iSocket, (void *)&header, sizeof(header));
						if (bytesRead == sizeof(header))
						{
							break;
						}
						else if (bytesRead < 0)
						{
							cppa::send(c.stdout, "Error, reading socket");
						}
						else if (bytesRead > 0)
						{
							cppa::send(c.stdout, "Error, couldnt parse full header", "Wanted", sizeof(header), "Got", bytesRead);
						}
						else
						{
							cppa::send(c.stdout, "Error, Read reached EOF", "Wanted", sizeof(header), "Got", bytesRead);
						}
						exit = 1;
						//cppa::self->quit();
						return;
					}
					
					nh(header);
					cppa::send(c.stdout, (int)header.version, (int)header.type, (int)header.requestID);
					cppa::actor_ptr handler;
					{
						auto handlerIt = handlers.find(header.requestID);
						if (handlerIt != handlers.end())
						{
							handler = (*handlerIt).second;
						}
						else
						{
							handler = newRequestHandler();
							handlers.emplace(header.requestID, handler);
						}
					}
					
					size_t dataBytes = header.length + header.padding;
					if (dataBytes)
					{
						record.data.reset(new uint8_t[dataBytes]);
						if ((bytesRead = read(iSocket, record.data.get(), dataBytes)) != dataBytes)
						{
							exit = 1;
							cppa::send(c.stdout, "Error, couldnt read full data.", "Got", bytesRead, "Wanted", dataBytes);
							continue;
						}
					}
					handler << recordTuple;
				}
			});
		}) << sockInfo;
		
	}
}

namespace comm
{
	namespace socket
	{
		struct SocketParams
		{
			int sock;
			sockaddr addr;
			socklen_t addrLen;
		};
		
		void startServer(SocketParams &);
		void stopServer(SocketParams &);
		void accept(const SocketParams & server, SocketParams & client);
	}
}
namespace fcgi
{
	
	struct system;
	struct SocketLayer
	{
		system * sys;
		cppa::actor_ptr reader;
		cppa::actor_ptr writer;
		SocketLayer(system * sys): sys(sys) {}
	};
	
	struct RequestMixLayer
	{
		system * sys;
		cppa::actor_ptr readerDemux;
		cppa::actor_ptr writerDemux;
		
		RequestMixLayer(system * sys): sys(sys) {}
	};
	
	
	struct system
	{
		std::shared_ptr<SocketLayer> socketLayer;
		std::shared_ptr<RequestMixLayer> requestLayer;
		
		system()
		{
			socketLayer.reset(new SocketLayer(this));
			requestLayer.reset(new RequestMixLayer(this));
		}
		
		void startListening(int port)
		{
			
		}
	};
}

int main()
{
	/*
	auto stdoutReceiver = cppa::spawn<cppa::blocking_api>([] () {
		cppa::receive_loop(cppa::others() >> [] {
			cppa::aout << cppa::to_string(cppa::self->last_dequeued()) << std::endl;
		});
	});
	
	fcgi::context ctx;
	ctx.stdout = stdoutReceiver;
	auto serverErrorReceiver = cppa::spawn([](cppa::actor_ptr out){
		cppa::become(cppa::on_arg_match >> [out](int & socket, sockaddr & addr, socklen_t & len){
			std::ostringstream msg;
			msg << "Error Socket = " << socket << std::endl;
			send(out, msg.str());
		});
	}, stdoutReceiver);
	
	//Create the socket server
	cppa::spawn<cppa::blocking_api>([&ctx] (cppa::actor_ptr errorReceiver) {
		
		fcgi::INetAddress addr(fcgi::INetAddress::any());
		addr.setPort(4785);
		fcgi::ServerSocket s(addr, fcgi::Semantics::stream, fcgi::Protocol::tcp);
		
		while(1)
		{
			cppa::cow_tuple<int, sockaddr, socklen_t> sendMsg;
			int & iSocket = cppa::get_ref<0>(sendMsg);
			sockaddr & addrCon = cppa::get_ref<1>(sendMsg);
			socklen_t & addrLenCon = cppa::get_ref<2>(sendMsg);
			addrLenCon = sizeof(addrCon);
			
			if ((iSocket = accept(s.getSocket(), &addrCon, &addrLenCon)) > 0)
				fcgi::newConnection(ctx, sendMsg);
			else
				errorReceiver << sendMsg;
		}
	}, serverErrorReceiver);
	 */
	
	cppa::await_all_others_done();
	cppa::shutdown();
	return 0;
}

template<typename T>
T * fcgi::logic::singleton<T>::m_globalObject;

const fcgi::INetAddress fcgi::INetAddress::s_any(fcgi::INetAddress::NamedAddressType::any);


