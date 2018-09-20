#ifndef _SENSOR_CONTROLLER_H_
#define _SENSOR_CONTROLLER_H_

namespace zmq {
	class context_t;
}

void sensor_dispatcher(zmq::context_t& zmq_context);
void sensor_printer(zmq::context_t& zmq_context);
void sensor_saver(zmq::context_t& zmq_context);

#endif
