#include <memory>
#include <stdexcept>
#include <string.h>
#include <iostream>
#include <thread>


#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>

#include <boost/asio.hpp>

#include <komm/fcgi.hpp>

using namespace boost::asio;
using namespace boost::asio::ip;
using namespace boost::system;

class Handler: public komm::fcgi::IRequestHandler {
public:
	void onAbort() {
		std::cout << "Handler got abort signal" << std::endl;
	}
	void onHeaders(std::map<std::string, std::string> & headers) {
		for (auto & tuple : headers) {
			std::cout << tuple.first << ":" << tuple.second;
		}
	}
};

class Factory : public komm::fcgi::IRequestHandlerFactory {
public:
	Factory() {
		std::cout << "Factory Created" << std::endl;
	}
	
	~Factory() {
		std::cout << "Factory Destroyed" << std::endl;
	}
	komm::fcgi::IRequestHandler * createHandler(komm::fcgi::IResponseWriter * writer) {
		return new Handler();
	}
};

class Server {
public:
	io_service io;
	tcp::acceptor acceptor;
	komm::fcgi::Server fcgiServer;
	
	std::shared_ptr<tcp::socket> currentSocket;
	bool abortRequested;
	
	Server()
	:acceptor(io, tcp::endpoint(tcp::v4(), 4785)),
	fcgiServer(io, std::shared_ptr<komm::fcgi::IRequestHandlerFactory>(new Factory))
	{
		accept();
	}
	
	void onConnection(const error_code & ec) {
		switch (ec.value()) {
			case boost::system::errc::operation_canceled:
				assert(abortRequested);
				abortRequested = false;
				return;
				break;
			case boost::system::errc::success:
				break;
			default:
				assert(false);
				break;
		}
		fcgiServer.manage(currentSocket);
		currentSocket.reset();
		accept();
	}
	
	void run() {
		std::cout << "Entering io.run()" << std::endl;
		io.run();
		std::cout << "Exiting io.run()" << std::endl;
	}
	
	void accept() {
		assert(!currentSocket);
		currentSocket = (std::make_shared<tcp::socket>(io));
		acceptor.async_accept(*currentSocket, [this](const error_code & ec) {
			this->onConnection(ec);
		});
	}
	
	void postAbort() {
		io.post([this] () {
			assert(!abortRequested);
			abortRequested = true;
			boost::system::error_code ec;
			acceptor.cancel(ec);
			
			fcgiServer.postAbort();
		});
	}
	
	
	~Server() {
		
	}
};

Server * gServer;

int main()
{
	{
		
		Server s;
		
		std::thread t([&s](){
			std::this_thread::sleep_for(std::chrono::seconds(4));
			s.postAbort();
		});
		s.run();
		t.join();
	}
	
	return 0;
}