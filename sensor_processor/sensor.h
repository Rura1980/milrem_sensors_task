#ifndef _SENSOR_H_
#define _SENSOR_H_

#include <cstdlib>
#include <unordered_map>
#include <shared_mutex>

#include <boost/circular_buffer.hpp>
#include <boost/optional.hpp>

enum value_type_t {
	NONE = 0x0000,
	FIELD_INT8 = 0x0001,
	FIELD_INT16 = 0x0002,
	FIELD_INT32 = 0x0003,
	FIELD_INT64 = 0x0004,
	FIELD_UINT8 = 0x0005,
	FIELD_UINT16 = 0x0006,
	FIELD_UINT32 = 0x0007,
	FIELD_UINT64 = 0x0008,
	FIELD_FLOAT = 0x0009,
	FIELD_DOUBLE = 0x000A,
	FIELD_CHARARRAY = 0x000B,
	FIELD_CHAR = 0x000C,
	FIELD_BOOL = 0x000D,
	FIELD_EMCY = 0x000E
};

struct sensor_stat_t {
	std::uint64_t last;
	std::uint64_t avg10;
	std::uint64_t avg;
	std::uint64_t total;
	boost::circular_buffer<std::uint64_t> last10;
};

using sensor_stat_map_t = std::unordered_map<std::uint8_t, sensor_stat_t>;

enum request_type_t {
	REQ_LAST = 0x0000,
	REQ_AVG10 = 0x0001,
	REQ_AVG = 0x0002
};

struct sensor_data_t {
	std::uint8_t id;
	std::uint8_t value_type;
	std::uint64_t reading;
};

struct sensor_req_t {
	std::uint8_t id;
	std::uint8_t request_type;
};

struct sensor_resp_t {
	std::uint8_t id;
	std::uint8_t request_type;
	std::uint64_t reading;
};

class sensor_stat_holder_t {
private:
	mutable std::shared_mutex access_mutex;
	sensor_stat_map_t sensor_stat_map;

	sensor_stat_holder_t();
	sensor_stat_holder_t(const sensor_stat_holder_t&) = delete;
	sensor_stat_holder_t& operator = (const sensor_stat_holder_t&) = delete;
public:
	static auto instance()->sensor_stat_holder_t&;
	void init();
	void add_sensor_data(std::uint8_t id, std::uint64_t reading);
	sensor_stat_map_t get_sensors_stat() const;
	boost::optional<sensor_stat_t> get_sensor_stat(std::uint8_t id) const;
};

sensor_req_t sensor_req_from_buffer(std::uint8_t * buffer);
void sensor_data_to_buffer(std::uint8_t * buffer, const sensor_data_t& data);
void sensor_resp_to_buffer(std::uint8_t * buffer, const sensor_resp_t& data);
sensor_resp_t sensor_resp_from_buffer(std::uint8_t * buffer);
sensor_data_t sensor_data_from_buffer(std::uint8_t * buffer);

std::uint64_t get_sensor_last_value(std::uint8_t id);
std::uint64_t get_sensor_avg10_value(std::uint8_t id);
std::uint64_t get_sensor_avg_value(std::uint8_t id);

#endif
