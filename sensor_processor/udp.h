#ifndef _UDP_H_
#define _UDP_H_

#include <cstdint>
#include <boost/array.hpp>
#include <boost/asio.hpp>

#include "zmq_common.h"

namespace zmq {
	class context_t;
}

using boost::asio::ip::udp;

class udp_sensor_server
{
public:
	udp_sensor_server(boost::asio::io_context& io_context, zmq::context_t& zmq_context, unsigned int port);
private:
	void start_receive();
	void handle_receive_sensor_data(const boost::system::error_code& error, std::size_t bytes_transferred);

	udp::socket sensor_socket_;
	zmqutils::socket_t push_socket_;
	udp::endpoint remote_endpoint_;
	boost::array<std::uint8_t, 10> recv_buffer_;
};

class udp_reqrep_server
{
public:
	udp_reqrep_server(boost::asio::io_context& io_context, unsigned short port);
private:
	void start_receive();
	void handle_receive( const boost::system::error_code& error, std::size_t /*bytes_transferred*/ );
	void handle_send(const boost::system::error_code& /*error*/, std::size_t /*bytes_transferred*/ );

	udp::socket socket_;
	udp::endpoint remote_endpoint_;
	boost::array<boost::uint8_t, 2> recv_buffer_;
	boost::array<boost::uint8_t, 10> send_buffer_;
};

void udp_client(boost::asio::io_context& io_context, const std::string& host, const std::string& port);

#endif
