# milrem_sensors_task
Milrem Robotics testtask

•	Have to compile in Windows and Linux
•	Solution in git
Task:
•	Sensor data is sent over UDP and broadcasted to port 12345. There are multiple sensors which send data at random intervals.
•	Data format: uint8 sensor number, uint8 value type, uint64 reading → payload length: 10B
•        Possible values for value type are defined in sensor_sender.py:14:28
•	Log the data from sensors and save it in csv format: timestamp, senor id, sensor reading
•	Print sensor data to standard output with frequency of 1 Hz, averaging over last 10 values
•	Sensor data can be requested by sending a UDP packet to port 12346
•        Request: uint8 sensor number, uint8 value type/kind (0 - last value, 1 - mean of last 10 values, 2 - mean of all values) → request length 2B
•        Response: uint8 sensor number, uint8 value type, uint64 reading → response length 10B

Solution description:

Solution is written in C++ using Visual Studio 2017 Community Edition v. 15.8.4
Solution uses 3d party libraries Boost.Asio 1.68 and zeromq 4.2.3

Solution is divided into several header and source files as the following:

	•  main.cpp - contains function main, which starts udp_sensor_server and udp_reqrep_server for udp connection communication handling. 
	              It also starts sensor_dispatcher, sensor_saver and sensor_printer functions in separate threads.
	•  udp.h, udp.cpp - contain udp connection handling classes udp_sensor_server and udp_reqrep_server. The first one is used to read sensor data from udp port 12345. 
	                    The second one listen for incoming request for sensor data on port 12346 and replies.
	•  sensor_controller.h, sensor_controller.cpp - contain sensor_dispatcher, sensor_saver and sensor_printer function definitions.
	•  sensor.h, sensor.cpp - constains utility functions for sensor data reading to/from buffer, sensor statistics computation etc

The algorithm looks like this:

udp_sensor_server receives sensor data(10 bytes) from udp socket(port 12345). 
udp_sensor_server pushes through zeromq PUSH socket this packet to sensor_dispatcher thread, and listens for the next packet.
sensor_dispatcher thread listens to zeromq PULL socket and receives sensor data packet from it. 
Then the sensors statistics is computed in sensor_stat_holder_t singleton class and saved into sensor_stat_map.
After doing that, sensor_dispatcher pushes sensor data through its zeromq PUSH socket to sensor_saver thread. 
sensor_saver thread pulls sensor data from zeromq PULL socket and appends socket data to sensors.csv file. 
sensor_dispatcher checks for new sensor data available and so on...

sensor_printer thread sleeps for 1 second and reads sensor statistics data from sensor_stat_holder_t singleton class and outputs it to the screen.
udp_reqrep_server listens for request on udp socket(port 12346). When it receives request, the corresponding statisics data is read from  sensor_stat_holder_t singleton class and is sent back to the udp client.

std::shared_mutex is used in sensor_stat_holder_t class to secure write/read access to the underlying map.
The decision was made not to read from sensors.csv file at all. The drawback is that after program restart, information about average sensor reading is not precise. 
But this can be solved by loading statistics data into sensor_stat_holder_t class on program start.

Visual Studio 2017 solution is provided but boost.props and zeromq_d.props files should be corrected to point to boost and zeromq includes and libs.
For simplicity compiled x64/Debug version is included with all needed libs and dlls for execution. 
Additionally Visual Studio 2017 x64 redistributable may be needed.
	
	


