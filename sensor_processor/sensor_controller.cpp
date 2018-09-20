#include "sensor_controller.h"
#include "zmq_common.h"
#include "sensor.h"

#include <ctime>
#include <iostream>
#include <fstream>

std::string make_daytime_string()
{
	using namespace std; // For time_t, time and ctime;
	time_t now = time(0);
	std::string formatted_time = ctime(&now);
	return formatted_time.substr(0, 24);
}

void sensor_dispatcher(zmq::context_t& zmq_context) {
	zmqutils::socket_t work_socket(zmq_context, ZMQ_PULL);
	zmqutils::socket_t saver_socket(zmq_context, ZMQ_PUSH);

	work_socket.bind("inproc://sensor_queue");
	saver_socket.bind("inproc://sensor_saver");

	//  Initialize poll set
	zmq::pollitem_t items[] = {
		{ work_socket.get(), 0, ZMQ_POLLIN, 0 }
	};
	while (true) {
		auto rc = zmq::poll(items, 1, -1);

		if (items[0].revents & ZMQ_POLLIN) {
			auto msg = work_socket.recvAsFrame();
			auto sensor_data_ = sensor_data_from_buffer(msg.data());
			sensor_stat_holder_t::instance().add_sensor_data(sensor_data_.id, sensor_data_.reading);
			saver_socket.send(msg);
		}
	}
}

void sensor_printer(zmq::context_t& zmq_context) {
	while (true) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
		auto sensors_stat = sensor_stat_holder_t::instance().get_sensors_stat();
		for (auto&& sensor_stat : sensors_stat) {
			std::cout << "sensor id = " << (int)sensor_stat.first << ", 10 last average = " << sensor_stat.second.avg10 << "\n";
		}
	}
}

void sensor_saver(zmq::context_t& zmq_context) {
	zmqutils::socket_t socket(zmq_context, ZMQ_PULL);
	socket.connect("inproc://sensor_saver");
	std::ofstream ofs("sensors.csv", std::ios::app);
	bool rc{ false };

	while (true) {
		auto buffer = socket.recvAsFrame();
		auto data = sensor_data_from_buffer(buffer.data());
		ofs << make_daytime_string() << "," << (int)data.id << "," << data.reading << "\n";
	}
	ofs.flush();
	ofs.close();
}
