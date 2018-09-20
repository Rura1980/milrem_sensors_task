#include <iostream>
#include <vector>
#include <chrono>

#include <boost/asio.hpp>

#include "zmq_common.h"
#include "udp.h"
#include "sensor_controller.h"

int main( int argc, char * argv[] ) {
	try
	{
		zmq::context_t zmq_context;
		boost::asio::io_context io_context;
		udp_sensor_server sensor_server(io_context, zmq_context, 12345);
		udp_reqrep_server reqrep_server(io_context, 12346);

		std::vector<std::thread> threads;

		threads.emplace_back([&zmq_context]() { sensor_printer(zmq_context); });
		threads.emplace_back([&zmq_context]() { sensor_saver(zmq_context); });
		threads.emplace_back([&zmq_context]() { sensor_dispatcher(zmq_context); });
		std::this_thread::sleep_for(std::chrono::seconds(1));
		//threads.emplace_back([&io_context]() { udp_client(io_context, "127.0.0.1", "12346"); });

		io_context.run();
		for (auto&& thr : threads)
			if (thr.joinable())
				thr.join();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	return 0;
}
