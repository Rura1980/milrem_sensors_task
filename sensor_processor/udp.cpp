#include "udp.h"
#include "zmq_common.h"
#include "sensor.h"

#include <boost/random.hpp>
#include <boost/bind.hpp>

udp_sensor_server::udp_sensor_server(boost::asio::io_context& io_context, zmq::context_t& zmq_context, unsigned int port)
	: sensor_socket_(io_context), push_socket_{ zmq_context, ZMQ_PUSH } 
{
	push_socket_.connect("inproc://sensor_queue");

	sensor_socket_.open(udp::v4());
	sensor_socket_.set_option(boost::asio::ip::udp::socket::reuse_address(true));
	sensor_socket_.bind(udp::endpoint(udp::v4(), port));

	start_receive();
}

void udp_sensor_server::start_receive()
{
	sensor_socket_.async_receive_from(
		boost::asio::buffer(recv_buffer_), remote_endpoint_,
		boost::bind(&udp_sensor_server::handle_receive_sensor_data, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
}

void udp_sensor_server::handle_receive_sensor_data( const boost::system::error_code& error, std::size_t bytes_transferred )
{
	if (!error)
	{
		push_socket_.send(zmqutils::TFrame(std::cbegin(recv_buffer_), std::cend(recv_buffer_)));
		start_receive();
	}
}

udp_reqrep_server::udp_reqrep_server(boost::asio::io_context& io_context, unsigned short port) : socket_(io_context, udp::endpoint(udp::v4(), port)) {
	start_receive();
}

void udp_reqrep_server::start_receive()
{
	std::cout << "start_receive" << std::endl;
	socket_.async_receive_from(
		boost::asio::buffer(recv_buffer_), remote_endpoint_,
		boost::bind(&udp_reqrep_server::handle_receive, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
}

void udp_reqrep_server::handle_receive(const boost::system::error_code& error, std::size_t /*bytes_transferred*/)
{
	std::cout << "handle_receive" << std::endl;
	if (!error)
	{
		auto sensor_data_req = sensor_req_from_buffer(recv_buffer_.data());
		std::cout << "sensor request id = " << (int)sensor_data_req.id << ", type = " << (int)sensor_data_req.request_type << "\n";

		sensor_resp_t sensor_data_resp;
		sensor_data_resp.id = sensor_data_req.id;
		sensor_data_resp.request_type = sensor_data_req.request_type;

		switch (sensor_data_req.request_type) {
		case request_type_t::REQ_LAST:
			sensor_data_resp.reading = get_sensor_last_value(sensor_data_req.id);
		case request_type_t::REQ_AVG10:
			sensor_data_resp.reading = get_sensor_avg10_value(sensor_data_req.id);
		case request_type_t::REQ_AVG:
			sensor_data_resp.reading = get_sensor_avg_value(sensor_data_req.id);
		}

		sensor_resp_to_buffer(send_buffer_.data(), sensor_data_resp);
		socket_.async_send_to(boost::asio::buffer(send_buffer_), remote_endpoint_,
			boost::bind(&udp_reqrep_server::handle_send, this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
		start_receive();
	}
}

void udp_reqrep_server::handle_send( const boost::system::error_code& /*error*/, std::size_t /*bytes_transferred*/)
{
}

void udp_client(boost::asio::io_context& io_context, const std::string& host, const std::string& port) {
	udp::resolver resolver(io_context);
	udp::resolver::query query(udp::v4(), host.c_str(), port.c_str());
	udp::endpoint receiver_endpoint = *resolver.resolve(query);
	udp::socket socket(io_context);
	socket.open(udp::v4());

	boost::random::mt19937 rng;
	boost::random::uniform_int_distribution<> range(0, 9);
	boost::random::uniform_int_distribution<> range2(0, 2);

	while (true) {
		std::uint8_t sensor_number = (std::uint8_t)range(rng);
		std::uint8_t sensor_request_type = (std::uint8_t)range2(rng);

		boost::array<std::uint8_t, 2> send_buf{ 0 };
		boost::array<std::uint8_t, 10> recv_buf{ 0 };

		memcpy(send_buf.c_array(), &sensor_number, sizeof(sensor_number));
		memcpy(send_buf.c_array() + sizeof(sensor_number), &sensor_request_type, sizeof(sensor_request_type));

		socket.send_to(boost::asio::buffer(send_buf), receiver_endpoint);
		socket.receive_from(boost::asio::buffer(recv_buf), receiver_endpoint);

		auto sensor_resp = sensor_resp_from_buffer(recv_buf.data());
		std::cout << "Request: " << (int)sensor_number << ", " << (int)sensor_request_type << "\n";
		std::cout << "Reply: " << (int)sensor_resp.id << ", " << (int)sensor_resp.request_type << ", " << sensor_resp.reading << "\n";
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}



