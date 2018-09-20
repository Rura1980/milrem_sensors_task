#include "sensor.h"

sensor_stat_holder_t& sensor_stat_holder_t::instance() {
	static sensor_stat_holder_t instance;
	return instance;
}

void sensor_stat_holder_t::init() {
}

sensor_stat_map_t sensor_stat_holder_t::get_sensors_stat() const {
	std::shared_lock<std::shared_mutex> rl(this->access_mutex);
	return this->sensor_stat_map;
}

boost::optional<sensor_stat_t> sensor_stat_holder_t::get_sensor_stat(std::uint8_t id) const {
	std::shared_lock<std::shared_mutex> rl(this->access_mutex);
	boost::optional<sensor_stat_t> sensor_stat;
	auto it = this->sensor_stat_map.find(id);
	if (it != end(this->sensor_stat_map))
		sensor_stat = it->second;
	return sensor_stat;
}

sensor_stat_holder_t::sensor_stat_holder_t() {
	this->init();
}

void sensor_stat_holder_t::add_sensor_data(std::uint8_t id, std::uint64_t reading) {
	{
		std::shared_lock<std::shared_mutex> rl(this->access_mutex);
		auto it = this->sensor_stat_map.find(id);
		if (it != end(this->sensor_stat_map)) {
			it->second.last = reading;
			if (it->second.total < 10) {
				if (reading > it->second.avg10)
					it->second.avg10 += (reading - it->second.avg10) / (it->second.total + 1);
				else
					it->second.avg10 -= (it->second.avg10 - reading) / (it->second.total + 1);
			}
			else {
				if (reading > it->second.last10.back())
					it->second.avg10 += (reading - it->second.last10.back()) / 10;
				else
					it->second.avg10 -= (it->second.last10.back() - reading) / 10;
			}

			if (reading > it->second.avg)
				it->second.avg += (reading - it->second.avg) / (it->second.total + 1);
			else
				it->second.avg -= (it->second.avg - reading) / (it->second.total + 1);

			it->second.last10.push_front(reading);
			++it->second.total;
			//std::cout << "sensor stat: id = " << (int)id << ", last = " << it->second.last << ", avg10 = " << it->second.avg10 << ", avg = " << it->second.avg << ", total = " << it->second.total << "\n";
			return;
		}
	}

	sensor_stat_t sensor_stat;
	sensor_stat.last = reading;
	sensor_stat.avg10 = reading;
	sensor_stat.avg = reading;
	sensor_stat.total = 1;
	sensor_stat.last10.resize(10);
	sensor_stat.last10.push_back(reading);
	{
		std::unique_lock<std::shared_mutex> rl(this->access_mutex);
		this->sensor_stat_map[id] = sensor_stat;
	}
	//std::cout << "sensor stat: id = " << (int)id << ", last = " << (int)sensor_stat.last << ", avg10 = " << sensor_stat.avg10 << ", avg = " << sensor_stat.avg << ", total = " << sensor_stat.total << "\n";
}

sensor_req_t sensor_req_from_buffer(std::uint8_t * buffer) {
	sensor_req_t req;
	memcpy(&req.id, buffer, sizeof(req.id));
	memcpy(&req.request_type, buffer + sizeof(req.id), sizeof(req.request_type));
	return req;
}

void sensor_data_to_buffer(std::uint8_t * buffer, const sensor_data_t& data) {
	memcpy(buffer, &data.id, sizeof(data.id));
	memcpy(buffer + sizeof(data.id), &data.value_type, sizeof(data.value_type));
}

void sensor_resp_to_buffer(std::uint8_t * buffer, const sensor_resp_t& data) {
	memcpy(buffer, &data.id, sizeof(data.id));
	memcpy(buffer + sizeof(data.id), &data.request_type, sizeof(data.request_type));
	memcpy(buffer + sizeof(data.id) + sizeof(data.request_type), &data.reading, sizeof(data.reading));
}

sensor_resp_t sensor_resp_from_buffer(std::uint8_t * buffer) {
	sensor_resp_t data;
	memcpy(&data.id, buffer, sizeof(data.id));
	memcpy(&data.request_type, buffer + sizeof(data.id), sizeof(data.request_type));
	memcpy(&data.reading, buffer + sizeof(data.id) + sizeof(data.request_type), sizeof(data.reading));

	return data;
}

sensor_data_t sensor_data_from_buffer(std::uint8_t * buffer) {
	sensor_data_t data;
	memcpy(&data.id, buffer, sizeof(data.id));
	memcpy(&data.value_type, buffer + sizeof(data.id), sizeof(data.value_type));

	std::uint8_t value_uint8_t{ 0 };
	std::uint16_t value_uint16_t{ 0 };
	std::uint32_t value_uint32_t{ 0 };
	std::uint64_t value_uint64_t{ 0 };
	std::int8_t value_int8_t{ 0 };
	std::int16_t value_int16_t{ 0 };
	std::int32_t value_int32_t{ 0 };
	std::int64_t value_int64_t{ 0 };

	switch (data.value_type) {
	case value_type_t::FIELD_UINT8:
		memcpy(&value_uint8_t, buffer + sizeof(data.id) + sizeof(data.value_type), sizeof value_uint8_t);
		data.reading = value_uint8_t;
		break;
	case value_type_t::FIELD_INT8:
		memcpy(&value_int8_t, buffer + sizeof(data.id) + sizeof(data.value_type), sizeof value_int8_t);
		data.reading = value_int8_t;
		break;
	case value_type_t::FIELD_UINT16:
		memcpy(&value_uint16_t, buffer + sizeof(data.id) + sizeof(data.value_type), sizeof value_uint16_t);
		data.reading = value_uint16_t;
		break;
	case value_type_t::FIELD_INT16:
		memcpy(&value_int16_t, buffer + sizeof(data.id) + sizeof(data.value_type), sizeof value_int16_t);
		data.reading = value_int16_t;
		break;
	case value_type_t::FIELD_UINT32:
		memcpy(&value_uint32_t, buffer + sizeof(data.id) + sizeof(data.value_type), sizeof value_uint32_t);
		data.reading = value_uint32_t;
		break;
	case value_type_t::FIELD_INT32:
		memcpy(&value_int32_t, buffer + sizeof(data.id) + sizeof(data.value_type), sizeof value_int32_t);
		data.reading = value_int32_t;
		break;
	case value_type_t::FIELD_UINT64:
		memcpy(&value_uint64_t, buffer + sizeof(data.id) + sizeof(data.value_type), sizeof value_uint64_t);
		data.reading = value_uint64_t;
		break;
	case value_type_t::FIELD_INT64:
		memcpy(&value_int64_t, buffer + sizeof(data.id) + sizeof(data.value_type), sizeof value_int64_t);
		data.reading = value_int64_t;
		break;
	default:
		data.reading = 0;
		break;
	}

	if (data.reading < 0)
		throw std::runtime_error("Value is less than zero: " + std::to_string(data.id) + ", " + std::to_string(data.value_type) + ", " + std::to_string(data.reading));

	return data;
}

std::uint64_t get_sensor_last_value(std::uint8_t id) {
	auto sensor_stat = sensor_stat_holder_t::instance().get_sensor_stat(id);
	if (sensor_stat)
		return sensor_stat.get().last;
	else
		return 0;
}

std::uint64_t get_sensor_avg10_value(std::uint8_t id) {
	auto sensor_stat = sensor_stat_holder_t::instance().get_sensor_stat(id);
	if (sensor_stat)
		return sensor_stat.get().avg10;
	else
		return 0;
}

std::uint64_t get_sensor_avg_value(std::uint8_t id) {
	auto sensor_stat = sensor_stat_holder_t::instance().get_sensor_stat(id);
	if (sensor_stat)
		return sensor_stat.get().avg;
	else
		return 0;
}