#include <vector>
#include <map>
#include <functional>
#include <istream>
#include <vector>
#include <thread>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "komm/fcgi.hpp"

namespace komm {
	namespace fcgi {
		
		namespace standard {
			
			enum class RecordType: uint8_t
			{
				BeginRequest = 1,
				AbortRequest,
				EndRequest,
				Params,
				StdIn,
				StdOut,
				StdErr,
				Data,
				GetValues,
				GetValuesResult,
				Unknown,
			};
			
			struct RecordHeader
			{
				uint8_t version;
				RecordType type;
				uint16_t requestID;
				uint16_t length;
				uint8_t padding;
				uint8_t unused;
			};
			
			struct RecordBody_Begin {
				uint16_t role;
				uint8_t flags;
				uint8_t unused[5];
			};
			
			struct RecordBody_End {
				uint32_t appStatus;
				uint8_t protocolStatus;
				uint8_t unused[3];
			};
			
		}
		
		
		class ServerImpl {
			
		public:
			ServerImpl(boost::asio::io_service & io, std::shared_ptr<IRequestHandlerFactory> factory);
			~ServerImpl();
			void postAbort();
			void manage(socket_ptr & socket);
			
		private:
			enum class SocketReadState {
				readHeader,
				readBody,
			};
			
			enum class RequestState {
				begun,
				paramed,
				stdined,
			};
			
			struct ManagedSocket;
			struct ManagedRequest {
				boost::uuids::uuid requestId;
				ManagedSocket * msock;
				uint16_t socketRequestId;
				std::shared_ptr<IResponseWriter> responseWriter;
				std::shared_ptr<IRequestHandler> requestHandler;
				RequestState state;
//
//				struct str_pair {
//					size_t szKey;
//					size_t szValue;
//					const char * pKey;
//					const char * pValue;
//				};
				
				std::map<std::string, std::string> paramMap;
				
			};
			
			class ResponseWriter: public komm::fcgi::IResponseWriter {
				ManagedRequest * mreq;
			public:
				ResponseWriter(ManagedRequest *);
			};
			
			struct ManagedSocket{
				boost::uuids::uuid socketId;
				socket_ptr socket;
				std::shared_ptr<IRequestHandlerFactory> requestHandlerFactory;
				SocketReadState readState;
				boost::asio::streambuf readStream;
				
				standard::RecordHeader header;
				union {
					standard::RecordBody_Begin begin;
					standard::RecordBody_End end;
				} body;
				bool shouldAbort;
				bool shouldAbortReadClosed;
				bool closeConnectionAfterRequest;
				std::map<uint16_t, ManagedRequest *> requestMap;
			};
			
			boost::asio::io_service & io;
			boost::uuids::random_generator uuidGen;
			std::shared_ptr<IRequestHandlerFactory> requestHandlerfactory;
			std::map<boost::uuids::uuid, std::unique_ptr<ManagedSocket>> socketMap;
			std::map<boost::uuids::uuid, std::unique_ptr<ManagedRequest>> requestMap;

			ManagedSocket * createManagedSocket(socket_ptr & socket);
			void destroyManagedSocket(ManagedSocket *);
			void abortManagedSocket(ManagedSocket *);
			ManagedRequest * createManagedRequest(ManagedSocket *);
			void destroyManagedRequest(ManagedRequest *);
			void abortManagedRequest(ManagedRequest *);
			
			void scheduleSocket(ManagedSocket *);
			void onReadComplete(ManagedSocket * socket, const boost::system::error_code & ec, size_t bytes);
			void onReadComplete_Inturrupted(ManagedSocket * socket, const boost::system::error_code & ec);
			void onReadComplete_Header(ManagedSocket * socket);
			void onReadComplete_Body(ManagedSocket * socket);
		};
	}
}




#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

using namespace komm::fcgi;
using namespace boost::system;
using namespace boost::asio;
using namespace boost::asio::ip;
using namespace boost::uuids;

static void byteSwap(uint16_t & val) {
	uint8_t * p = reinterpret_cast<uint8_t *>(&val);
	std::swap(p[0], p[1]);
}

static void byteSwap(uint32_t & val) {
	uint8_t * p = reinterpret_cast<uint8_t *>(&val);
	std::swap(p[0], p[3]);
	std::swap(p[1], p[2]);
}

static void byteSwap(uint64_t & val) {
	uint8_t * p = reinterpret_cast<uint8_t *>(&val);
	std::swap(p[0], p[7]);
	std::swap(p[1], p[6]);
	std::swap(p[2], p[5]);
	std::swap(p[3], p[4]);
}

static size_t readVariant(std::istream & ip, uint32_t & out) {
	out = 0;
	ip.read((char *)&out, 1);
	if (out >> 7) {
		ip.read((char *)&out + 1, 3);
		byteSwap(out);
		return 4;
	}
	return 1;
};

static size_t readVariant(const char * b, uint32_t & out) {
	out = b[0];
	if (out >> 7) {
		out = *(uint32_t *)b;
		byteSwap(out);
		return 4;
	}
	return 1;
};

Server::Server (boost::asio::io_service & io, std::shared_ptr<IRequestHandlerFactory> factory): m_impl(new ServerImpl(io, factory)){
}

Server::~Server() {
	
}

void Server::postAbort() {
	m_impl->postAbort();
}
void Server::manage (socket_ptr & socket) {
	m_impl->manage(socket);
}

ServerImpl::ServerImpl(io_service & io, std::shared_ptr<IRequestHandlerFactory> factory): io(io), requestHandlerfactory(factory){
	
	static_assert(sizeof(standard::RecordHeader) == 8, "Structs not compact");
	static_assert(sizeof(standard::RecordBody_Begin) == 8,"Structs not compact");
	static_assert(sizeof(standard::RecordBody_End) == 8,"Structs not compact");

	
}
void ServerImpl::postAbort() {
	
	if(requestMap.size()) {
		io.post([this](){
			for(auto & sock: socketMap) {
				abortManagedSocket(sock.second.get());
			}
		});
	}
	
}
ServerImpl::~ServerImpl() {

}

void ServerImpl::onReadComplete_Body(ManagedSocket * socket) {
	standard::RecordHeader & h(socket->header);
	//Unpack the bytes
	socket->readStream.commit(h.length + h.padding);
	
	

	socket->readState = SocketReadState::readHeader;
	scheduleSocket(socket);

	switch (h.type) {
		case standard::RecordType::BeginRequest:{
			standard::RecordBody_Begin & begin = socket->body.begin;
			{
				const size_t szBegin = sizeof(standard::RecordBody_Begin);
				assert(h.length  == szBegin);
				std::istream is(&socket->readStream);
				is.read((char *)&socket->body.begin, szBegin);
			}
			byteSwap(begin.role);
			assert(begin.role == 1);
			if (begin.flags & 1) {
				socket->closeConnectionAfterRequest = false;
			}
			createManagedRequest(socket);
			break;
		}
		case standard::RecordType::Params:{
			std::istream is(&socket->readStream);
			auto mreq = socket->requestMap[h.requestID];
			size_t bytesRead = 0;
			while(bytesRead < h.length) {
				uint32_t szKey, szValue;
				bytesRead += readVariant(is, szKey);
				bytesRead += readVariant(is, szValue);
				char * cKey = new char[szKey];
				is.read(cKey, szKey);
				std::string sKey(cKey, szKey);
				bytesRead += szKey;
				delete cKey;
				char * cValue = new char[szValue];
				is.read(cValue, szValue);
				bytesRead += szValue;
				std::string sValue(cValue, szValue);
				delete cValue;
				mreq->paramMap.emplace(std::move(sKey), std::move(sValue));
			}
			mreq->requestHandler->onHeaders(mreq->paramMap);
			break;
		}
			
		case standard::RecordType::AbortRequest:{
			break;
		}
			
		default:{
			assert(false);
		}
	}
	socket->readStream.consume(h.length + h.padding);
}

void ServerImpl::onReadComplete_Header(ManagedSocket * socket) {
	standard::RecordHeader & h(socket->header);
	
	//Unpack the bytes
	{
		const size_t szHeader = sizeof(standard::RecordHeader);
		socket->readStream.commit(szHeader);
		std::istream is(&socket->readStream);
		is.read((char *)&socket->header, szHeader);
		socket->readStream.consume(szHeader);
	}
	byteSwap(socket->header.requestID);
	byteSwap(socket->header.length);
	
	//If there is a body expected next, set the state appropriately and then we can dispatch the next read.
	if ((h.length + h.padding) != 0) {
		socket->readState = SocketReadState::readBody;
	}
	scheduleSocket(socket);
	
	
	std::cout << to_string(socket->socketId) << ": Got Header, version " << (int)h.version << ", type " << (int)h.type << ", requestID " << (int)h.requestID << ", length " << (int)h.length << std::endl;
	
	
	switch (h.type) {

		case standard::RecordType::BeginRequest:{
			assert(h.length == sizeof(standard::RecordBody_Begin));
			
			break;
		};
		case standard::RecordType::Params:{
			break;
		}
		case standard::RecordType::StdIn:{
			break;
		}
		case standard::RecordType::AbortRequest:{
			break;
		}
		default:{
			assert(false);
		}
	}
}
void ServerImpl::onReadComplete_Inturrupted(ManagedSocket * socket, const boost::system::error_code & ec) {
	if (socket->shouldAbort) {
		//Assert so change in behaviour is noticed. This error is returned whene socket read is shutdown
		//EOF
		assert(ec.value() == errc::address_in_use);
		socket->shouldAbortReadClosed = true;
		for (auto & req : socket->requestMap) {
			abortManagedRequest(req.second);
		}
	}
	
	std::cout << to_string(socket->socketId) << ": Error " <<  ec.message() << std::endl;
	destroyManagedSocket(socket);
	return;

}

void ServerImpl::onReadComplete(ManagedSocket * socket, const boost::system::error_code & ec, size_t bytes) {
	if (ec){
		onReadComplete_Inturrupted(socket, ec);
	}
	switch (socket->readState) {
		case SocketReadState::readHeader:
		{
			assert(bytes == sizeof(standard::RecordHeader));
			onReadComplete_Header(socket);
			break;
		}
		case SocketReadState::readBody:
		{
			standard::RecordHeader & h(socket->header);
			assert(bytes == (h.length + h.padding));
			onReadComplete_Body(socket);
			break;
		}
	}
}

void ServerImpl::scheduleSocket(ManagedSocket * socket) {
	switch (socket->readState) {
		case SocketReadState::readHeader:
		{
			auto cb = std::bind(&ServerImpl::onReadComplete, this, socket, std::placeholders::_1, std::placeholders::_2);
			async_read(*socket->socket, socket->readStream.prepare(sizeof(standard::RecordHeader)), cb);
			break;
		}
		case SocketReadState::readBody:
			standard::RecordHeader & h(socket->header);
			auto cb = std::bind(&ServerImpl::onReadComplete, this, socket, std::placeholders::_1, std::placeholders::_2);
			async_read(*socket->socket, socket->readStream.prepare(h.length + h.padding), cb);
			break;
			
	};
}

ServerImpl::ManagedSocket * ServerImpl::createManagedSocket(socket_ptr & socket) {
	auto pair = socketMap.emplace(uuidGen(), std::unique_ptr<ManagedSocket>(new ManagedSocket()));
	assert(pair.second);
	
	const uuid & key = pair.first->first;
	ManagedSocket & msock = *pair.first->second.get();
	msock.socketId = key;
	msock.socket = socket;
	msock.readState = SocketReadState::readHeader;
	msock.closeConnectionAfterRequest = true;
	msock.requestHandlerFactory = requestHandlerfactory;
	std::cout << "SOCK+" << to_string(msock.socketId) << std::endl;
	return &msock;
}
void ServerImpl::destroyManagedSocket(komm::fcgi::ServerImpl::ManagedSocket * msock) {
	std::cout << "SOCK-" << to_string(msock->socketId) << std::endl;
	socketMap.erase(msock->socketId);
}

void ServerImpl::abortManagedSocket(ManagedSocket * msock) {
	msock->shouldAbort = true;
	msock->socket->shutdown(boost::asio::socket_base::shutdown_type::shutdown_receive);
}


ServerImpl::ManagedRequest * ServerImpl::createManagedRequest(komm::fcgi::ServerImpl::ManagedSocket * socket) {
	auto pair = requestMap.emplace(uuidGen(), std::unique_ptr<ManagedRequest>(new ManagedRequest()));
	assert(pair.second);
	
	const uuid & key = pair.first->first;
	ManagedRequest & mreq = *pair.first->second.get();
	mreq.requestId = key;
	mreq.msock = socket;
	mreq.socketRequestId = socket->header.requestID;
	mreq.state = RequestState::begun;
	socket->requestMap.emplace(mreq.socketRequestId, &mreq);
	
	mreq.responseWriter.reset(new ResponseWriter(&mreq));
	mreq.requestHandler.reset(socket->requestHandlerFactory->createHandler(mreq.responseWriter.get()));
	std::cout << "RQST+" << to_string(mreq.requestId) << std::endl;
	return &mreq;
}


void ServerImpl::destroyManagedRequest(ManagedRequest * mreq) {
	std::cout << "RQST-" << to_string(mreq->requestId) << std::endl;
	mreq->msock->requestMap.erase(mreq->socketRequestId);
	requestMap.erase(mreq->requestId);
}

void ServerImpl::abortManagedRequest(ManagedRequest * mreq) {
	//The first step in abort sequence is to close the read of the socket
	if (!mreq->msock->shouldAbortReadClosed)
		return;
	
	mreq->requestHandler->onAbort();
}

void ServerImpl::manage(socket_ptr & socket) {
	auto sock = createManagedSocket(socket);
	scheduleSocket(sock);
}

ServerImpl::ResponseWriter::ResponseWriter(ManagedRequest * mreq): mreq(mreq) {
	
}






