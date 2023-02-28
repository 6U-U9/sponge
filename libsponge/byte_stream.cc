#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) { 
    this->_capacity = capacity;
}

size_t ByteStream::write(const string &data) {
    size_t index = 0;
    while(this->buffer.size() < this->_capacity && index < data.length())
    {
        this->buffer.push_back(data[index]);
        index ++;
        this->writecount ++;
    }
    return index;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    size_t l = min(len, this->buffer.size());
    auto str = std::string(l, 0);
    size_t index = 0;
    for(auto p=this->buffer.begin(); p!=this->buffer.end(); p++)
    {
        if(index >= l)
            break;
        str[index] = *p;
        index++;
    }
    return str;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    size_t index = 0;
    while(this->buffer.size()  > 0 && index < len)
    {
        this->buffer.pop_front();
        index ++;
        this->readcount ++;
    }
 }

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    auto str = peek_output(len);
    pop_output(len);
    return str;
}

void ByteStream::end_input() {
    this->ended = true;
}

bool ByteStream::input_ended() const { 
    return this->ended;
}

size_t ByteStream::buffer_size() const { return this->buffer.size(); }

bool ByteStream::buffer_empty() const { return this->buffer.empty(); }

bool ByteStream::eof() const { return input_ended() && buffer_empty(); }

size_t ByteStream::bytes_written() const { return this->writecount; }

size_t ByteStream::bytes_read() const { return this->readcount; }

size_t ByteStream::remaining_capacity() const { return this->_capacity - this->buffer.size(); }
