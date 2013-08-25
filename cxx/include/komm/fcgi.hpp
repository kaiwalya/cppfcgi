#if !defined(KOMM_FCGI_HPP)
#define KOMM_FCGI_HPP

#include <functional>
#include <vector>
#include <future>
#include <map>

#include <boost/asio.hpp>

namespace flow {

	struct schedular {
		virtual void schedule(std::function<void()> & f) = 0;
		virtual ~schedular();
	};
	
	class flow {
	public:
		flow (schedular *, std::function<void()> &);
		flow (schedular *, std::function<void()> &&);
		flow then (schedular *, std::function<void()> &);
		flow then (schedular *, std::function<void()> &&);
		flow join (flow, ...);
		~flow();
	};
}

namespace komm {
	
	namespace fcgi {
		typedef std::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr;
		class ServerImpl;
		
		class IResponseWriter {
		public:
			virtual ~IResponseWriter(){};
		};
		
		class IRequestHandler {
		public:
			virtual void onHeaders(std::map<std::string, std::string> &) = 0;
			virtual void onAbort() = 0;
			virtual ~IRequestHandler(){};
		};
		
		class IRequestHandlerFactory {
		public:
			virtual IRequestHandler * createHandler(IResponseWriter *) = 0;
			virtual ~IRequestHandlerFactory() {};
		};
		
		class Server {
			std::unique_ptr<ServerImpl> m_impl;
		private:
		public:
			Server(boost::asio::io_service & io, std::shared_ptr<IRequestHandlerFactory> factory);
			~Server();
			void postAbort();
			void manage(socket_ptr & socket);
		};
	}
}

#endif
