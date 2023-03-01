#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPConnection::send() {
    if(_closed) return;
    while(!_sender.segments_out().empty())
    {
        auto seg = _sender.segments_out().front();
        _sender.segments_out().pop();
        if(_receiver.ackno().has_value())
        {
            seg.header().ack = true;
            seg.header().ackno = _receiver.ackno().value();
        }
        size_t max_win = numeric_limits<uint16_t>().max();
        seg.header().win = min(_receiver.window_size(), max_win);
        this->segments_out().push(seg);
    }

}

void TCPConnection::reset(bool notice) {
    _linger_after_streams_finish = false;
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();
    if(notice)
    {
        while (!_sender.segments_out().empty()) {
            // pop all segments
            _sender.segments_out().pop();
        }
        _sender.send_empty_segment();
        TCPSegment& seg = _sender.segments_out().front();
        seg.header().rst = true;
        send();
    }
}

size_t TCPConnection::remaining_outbound_capacity() const { 
    return _sender.stream_in().remaining_capacity();
 }

size_t TCPConnection::bytes_in_flight() const { 
    return _sender.bytes_in_flight();
}

size_t TCPConnection::unassembled_bytes() const {
    return _receiver.unassembled_bytes();
 }

size_t TCPConnection::time_since_last_segment_received() const { 
    return _time - _last_ack_time; 
}

void TCPConnection::segment_received(const TCPSegment &seg) {
    if(seg.header().syn)
        _syn = true;
    if (seg.header().rst) 
        reset(false);
    if(!_syn)
        return;
    _last_ack_time = _time;
    _receiver.segment_received(seg);
    if (_receiver.stream_out().input_ended() && !_sender.stream_in().eof()) {
        // passive close
        _linger_after_streams_finish = false;
    }
    if(seg.header().ack)
        _sender.ack_received(seg.header().ackno, seg.header().win);
    _sender.fill_window();
    if (_receiver.ackno().has_value() && _sender.segments_out().empty()) 
    {   
        if(seg.length_in_sequence_space() > 0) 
            _sender.send_empty_segment();
        if((seg.length_in_sequence_space() == 0) and seg.header().seqno == _receiver.ackno().value() - 1)
            _sender.send_empty_segment();
    }
    send();
 }

bool TCPConnection::active() const { 
    if(!_receiver.stream_out().error() && !_receiver.stream_out().input_ended() ) 
        return true;
     if(!_sender.stream_in().error() && (!_sender.stream_in().eof()  || _sender.bytes_in_flight() > 0))
        return true;   
    if(_linger_after_streams_finish && _time < _last_ack_time + _cfg.rt_timeout * 10)
        return true;
    return false;
}

size_t TCPConnection::write(const string &data) {
    size_t len = _sender.stream_in().write(data);
    _sender.fill_window();
    send();
    return len;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) { 
    _time+= ms_since_last_tick;
    _sender.tick(ms_since_last_tick);
    if(!active())
        _closed = true;

    if(_sender.consecutive_retransmissions() > _cfg.MAX_RETX_ATTEMPTS)
        reset(true);
    send();
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    send();
}

void TCPConnection::connect() {
    if(_syn)
        return;
    _sender.fill_window();
    send();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            reset(true);
            // Your code here: need to send a RST segment to the peer
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
