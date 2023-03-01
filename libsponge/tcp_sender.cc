#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _timeout{retx_timeout} {}

uint64_t TCPSender::bytes_in_flight() const { return _next_seqno - _last_ack; }

void TCPSender::fill_window() {
    uint64_t remain = (_window <=0 ? 1 : _window) - bytes_in_flight();
    bool sent = false;
    while(remain > 0 && !_fin)
    {
        TCPSegment seg;
        if(_next_seqno == 0)
        {
            seg.header().syn  = true;
            remain --;
        }
        uint64_t length = min(remain,  TCPConfig::MAX_PAYLOAD_SIZE);
        string payload = _stream.read(length);
        seg.payload() = move(payload);
        seg.header().seqno = wrap(_next_seqno, _isn);
        _next_seqno += seg.length_in_sequence_space();
        remain -= seg.payload().size();
        if (_stream.eof() && remain > 0 && ! _fin) 
        {
            seg.header().fin = true;
            _next_seqno += 1;
            _fin = true;
            remain--;
        }
        if(seg.length_in_sequence_space() == 0)
            break;
        _segments_out.push(seg);
        _not_ack_segments.push(seg);
        sent = true;
    }
    if(sent && !countingdown)
    {
        countingdown = true;
        _timer = 0;
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
    uint64_t ack_seqno = unwrap(ackno, _isn, _last_ack);
    if(ack_seqno < _last_ack)
        return;
    if(ack_seqno > _next_seqno)
        return;
    _window = window_size;
    if(ack_seqno == _last_ack)
        return;
    _timer = 0;
    _timeout = _initial_retransmission_timeout;
    _consecutive_retransmissions = 0;
    while (!_not_ack_segments.empty()) {
        TCPSegment seg = _not_ack_segments.front();
        if (seg.length_in_sequence_space() + unwrap(seg.header().seqno, _isn, _last_ack) <= ack_seqno) {
            _not_ack_segments.pop();
        } else {
            break;
        }
    }
    _last_ack = ack_seqno;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    if(_not_ack_segments.empty() || !countingdown)
    {
        countingdown = false;
         _timer = 0;
         return;
    }
    _timer += ms_since_last_tick;
    if(_timer >=_timeout)
    {
        if(_window != 0)
        {
            _timeout *= 2;
            _consecutive_retransmissions ++;
            //cerr<<_consecutive_retransmissions<<endl;
        }
        _timer = 0;
        TCPSegment seg = _not_ack_segments.front();
        _segments_out.push(seg);
    }
 }

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = wrap(_next_seqno, _isn);
    //_not_ack_segments.push(seg);
    _segments_out.push(seg);
}
