#include "wrapping_integers.hh"

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    n = n + isn.raw_value();
    return WrappingInt32{static_cast<uint32_t>(n & WrappingInt32::mask)};
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    uint64_t cp = checkpoint & WrappingInt32::mask;
    uint64_t num = n.raw_value();
    uint64_t init = isn.raw_value();
    uint64_t round =  WrappingInt32::mask + 1L;
    if(num >= init)   {
        num = num - init;
    }
    else{
        num = num + round  - init;
    }
    cp = checkpoint - cp + num;
    if(cp >= checkpoint)
    {
        if(cp >= round && cp - checkpoint > checkpoint + round - cp)
            return cp - round;
        return cp;
    }
    else
    {
        if(UINT64_MAX - cp >= round && cp + round - checkpoint < checkpoint - cp)
            return cp  + round;
        return cp;
    }
}
