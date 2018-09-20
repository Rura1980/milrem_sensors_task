#include "zmq_common.h"
#include <iomanip>
#include <stdexcept>

bool interrupted = false;

void set_interrupted( bool value ) {
	interrupted = value;
};

bool get_interrupted() {
	return interrupted;
};

namespace zmqutils {
    socket_t::socket_t( zmq::context_t& context, int type ) : socket_( context, type ) {
    };

    socket_t::socket_t( zmq::context_t& context, int type, const std::string& id ) : socket_( context, type ) {
        this->socket_.setsockopt( ZMQ_IDENTITY, id.c_str(), id.length() );
    };

    zmq::socket_t& socket_t::get() {
        return this->socket_;
    };

    void socket_t::connect( const std::string& addr ) {
        this->socket_.connect( addr.c_str() );
    };
    
    void socket_t::bind( const std::string& addr ) {
        this->socket_.bind( addr.c_str() );
    };

    void socket_t::setLinger( int linger ) {
        this->socket_.setsockopt( ZMQ_LINGER, &linger, sizeof(linger) );
    };
	
	void socket_t::setoption( int option_, const void *optval_, size_t optvallen_ ) {
		this->socket_.setsockopt( option_, optval_, optvallen_ );
	};

    bool socket_t::send( const std::string& frame, int flags ) {
        return this->send( (const unsigned char *)frame.c_str(), frame.size(), flags );
    };

    bool socket_t::send( const TFrame& frame, int flags ) {
        if (frame.empty())
            return this->send( (const unsigned char *)NULL, 0, flags );
        else
            return this->send( &frame[0], frame.size(), flags );
    };

	bool socket_t::send( boost::shared_ptr<TFrames> msg ) {
		if(msg->empty())
			throw std::runtime_error("No frames detected");

         // all frames but last one
		for( TFrames::size_type i = 0; i < msg->size() - 1; ++i )
			if(!this->send( msg->at(i), ZMQ_SNDMORE ) )
				return false;
        // last frame
        return this->send( msg->back() );
    };

    bool socket_t::send( int value, int flags ) {
        zmq::message_t msg( sizeof(value) );
        memcpy( msg.data(), &value, sizeof(value) );
        return this->socket_.send( msg, flags );
    };

    bool socket_t::send( boost::uint8_t value, int flags ) {
        zmq::message_t msg( sizeof(value) );
        memcpy( msg.data(), &value, sizeof(value) );
        return this->socket_.send( msg, flags );
    }

	TFrames socket_t::recv() {
		TFrames frames;
		do {
			frames.push_back( this->recv( 0 ) );		
        } 
		while( this->sockopt_rcvmore() );

        return frames;
    }

    std::string socket_t::recvAsString( int flags ) {
        return frame2str( this->recv( flags ) );
    }

    int socket_t::recvAsInt( int flags ) {
        zmq::message_t message;
        this->recv( message, flags );
        return *(static_cast<int*>(message.data()));
    }

    boost::optional<boost::uint8_t> socket_t::recvAsByte( int flags ) {
        zmq::message_t message;
        auto rc = this->recv( message, flags );

		boost::optional<boost::uint8_t> result;
		if (!rc)
			return result;
		else
			return *(static_cast<boost::uint8_t*>(message.data()));
    }  

	TFrame socket_t::recvAsFrame(int flags) {
		return this->recv(flags);
	}

    bool socket_t::sockopt_rcvmore() {
        boost::int64_t rcvmore(0);
        size_t type_size = sizeof(boost::int64_t);
        this->socket_.getsockopt( ZMQ_RCVMORE, &rcvmore, &type_size );
        return (rcvmore != 0);
    }

    bool socket_t::send( const unsigned char * frame, size_t len, int flags ) {
        zmq::message_t msg( len );
        memcpy( msg.data(), frame, len );
        return this->socket_.send( msg, flags );
    }

    TFrame socket_t::recv( int flags ) {
        zmq::message_t message;
        auto rc = this->recv( message, flags );

		if (!rc) // EAGAIN
			return TFrame{};
 
        const unsigned char * base = static_cast<const unsigned char*>(message.data());
        return TFrame( base, base + message.size() );
    }

    bool socket_t::recv( zmq::message_t& message, int flags ) {
       return this->socket_.recv( &message, flags );
    }

    //-----------------------------------------------------------------------

    std::string frame2str( const TFrame& frame ) {
        std::string result_;
        result_.assign( frame.begin(), frame.end() );
        return result_;
    };

    TFrame str2frame( const std::string& s ) {
        TFrame frame_;
        frame_.assign( s.begin(), s.end() );
        return frame_;
    };

}

std::ostream& operator << ( std::ostream& os, const zmqutils::TFrame& frame ) {
	// Dump the message as text or binary
	bool is_text(true);
	for (unsigned int char_nbr = 0; char_nbr < frame.size(); ++char_nbr)
        is_text = is_text && !(frame [char_nbr] < 32 || frame [char_nbr] > 127);

	for (unsigned int char_nbr = 0; char_nbr < frame.size(); ++char_nbr) {
		if (is_text) {
			os << (char) frame[char_nbr];
		}
		else {
			os << std::hex << std::setw(2) << std::setfill('0') << (short int) frame[char_nbr];
		}
	}
	os.unsetf( std::ios::hex );
	return os;
}

std::ostream& operator << ( std::ostream& os, const zmqutils::TFrames& msg ) {
	os << "--------------------------------------" << "\n";
	for (unsigned int framenum = 0; framenum < msg.size(); ++framenum) {
		const zmqutils::TFrame& frame = msg.at(framenum);
		os << "[" << std::setw(5) << std::setiosflags(std::ios::right) << std::setfill('0') << std::dec << (int) frame.size() << "] " << std::resetiosflags(std::ios::right);
		os << frame << "\n";
	}
	os << "--------------------------------------" << "\n";
	os.flush();
	os.unsetf( std::ios::hex );
	return os;
}
