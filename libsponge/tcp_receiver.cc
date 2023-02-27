#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    if(!_isn.has_value())
    {
        if(seg.header().syn)
            _isn = seg.header().seqno;
        else
        {
           std:: cerr << "connection not exist"<< std::endl;
           return;
        }
    }
    bool eof = seg.header().fin;
    std::string payload = seg.payload().copy();
    uint64_t index = unwrap(seg.header().seqno , _isn.value(), this->_reassembler.expect());
    if(!seg.header().syn)
        index --;
    this->_reassembler.push_substring(payload, index, eof);
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    if (!_isn.has_value())
        return std::nullopt;
    return wrap(this->_reassembler.expect()+1,  _isn.value()) + this->stream_out().input_ended();
 }

size_t TCPReceiver::window_size() const { return this->_capacity - this->stream_out().buffer_size(); }
