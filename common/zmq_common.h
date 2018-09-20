#ifndef _ZMQ_COMMON_H_
#define _ZMQ_COMMON_H_

#include <iosfwd>
#include <string>
#include <deque>
#include <vector>
#include <sstream>

#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/optional.hpp>
#include <boost/array.hpp>

#include <zmq.hpp>

void set_interrupted( int value );
bool get_interrupted();

namespace zmqutils {
    typedef std::vector<boost::uint8_t> TFrame;
    typedef std::deque<TFrame> TFrames;

    const TFrame EMPTY_FRAME;

    class socket_t {
    private:
        zmq::socket_t socket_;

        socket_t( const socket_t& ) = delete;
        socket_t& operator = ( const socket_t& ) = delete;

        bool sockopt_rcvmore();
        TFrame recv( int flags );
		bool recv( zmq::message_t& message, int flags );
    public:  
        socket_t( zmq::context_t& context, int type );
        socket_t( zmq::context_t& context, int type, const std::string& identity );
        zmq::socket_t& get();

        void connect( const std::string& addr );
        void bind( const std::string& addr );

        void setLinger( int linger = -1 );
        void setoption( int option_, const void *optval_, size_t optvallen_ );

        bool send( const std::string& frame, int flags = 0 );
        bool send( const TFrame& frame, int flags = 0 );
        bool send( boost::shared_ptr<TFrames> msg );
        bool send( int value, int flags = 0 );
        bool send( boost::uint8_t value, int flags = 0 );
		bool send( const unsigned char * frame, size_t len, int flags = 0 );
        TFrames recv();
        std::string recvAsString( int flags = 0 );
        int recvAsInt( int flags = 0 );
		boost::optional<boost::uint8_t> recvAsByte( int flags = 0 );
		TFrame recvAsFrame(int flags = 0);

		template<typename T>
		T recvAs(int flags = 0);

		template<typename T>
		bool sendAs(T data, int flags = 0);

		template<int N>
		bool send( const boost::array<std::uint8_t,N>& buffer, int flags = 0);
	};

	template <typename T>
	T socket_t::recvAs(int flags) {
		zmq::message_t message;
		this->recv(message, flags);
		return *(static_cast<T*>(message.data()));
	}

	template <typename T>
	bool socket_t::sendAs( T data, int flags) {
		zmq::message_t msg(sizeof T);
		memcpy(msg.data(), &data, sizeof T);
		return this->socket_.send(msg, flags);
	}

	template<int N>
	bool socket_t::send(const boost::array<std::uint8_t, N>& buffer, int flags ) {
		zmq::message_t msg(N);
		memcpy(msg.data(), buffer.data(), N);
		return this->socket_.send(msg, flags);
	}

    std::string frame2str( const TFrame& frame );
    TFrame str2frame( const std::string& s );

    template <typename T>
    T unpack( const void * data, const size_t datasize ) {
        T obj_;
        if (data && datasize) {
            if (obj_.ParseFromArray( data, datasize ))
                ;//printf ("%s unpacked\n", obj_.DebugString().c_str());
            else
                ;//printf ("%s unpacked with errors\n", obj_.DebugString().c_str());
        }
        return obj_;
    };
    template <typename T>
    T unpack(TFrame& payload ) {
        const void * data = (!payload.empty())?(&payload[0]):NULL;
        return unpack<T>( data, payload.size() );
    };

    template <typename T>
    boost::shared_ptr<TFrames> pack( const T& obj ) {
        auto msg_ = boost::make_shared<TFrames>();
        zmqutils::TFrame buffer_;
        if (obj.ByteSize() > 0) {
            buffer_.resize( obj.ByteSize(), ' ' );
            bool rc = obj.SerializeToArray( &buffer_[0], buffer_.size() );
            if (rc)
                msg_->push_back( buffer_ );
        }
        else
            msg_->push_back( buffer_ );

        return msg_;
    };

    template <typename T>
    bool pack( const T& obj, TFrame& payload ) {
        if (obj.ByteSize() > 0) {
            payload.resize( obj.ByteSize(), ' ' );
            bool rc = obj.SerializeToArray( &payload[0], payload.size() );
            return rc;
        }
        else {
            payload.clear();
            return true;
        }
    };
};

std::ostream& operator << ( std::ostream& os, const zmqutils::TFrame& msg );
std::ostream& operator << ( std::ostream& os, const zmqutils::TFrames& msg );

#endif
