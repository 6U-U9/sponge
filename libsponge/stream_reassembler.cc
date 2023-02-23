#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity),buffer(capacity),arrived(capacity){}

void StreamReassembler::try_output()
{
    std::string output = "";
    for(size_t i=0; i<this->arrived.size(); i++)
    {
        if(!arrived[i])
            break;
        output.push_back(buffer[i]);
    }
    size_t wroten = this->_output.write(output);
    for(size_t i=0; i<wroten; i++)
    {
        this->buffer.pop_front();
        this->arrived.pop_front();
        this->_unassembled_bytes --;
        this->expect_index ++;
        this->buffer.push_back(0);
        this->arrived.push_back(false);
    }
    if(this->expect_index == this->_eof)
        this->_output.end_input();
}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    for(size_t i=0; i<data.length(); i++)
    {
        if(i+index-this->expect_index < this->arrived.size() && this->arrived[i+index-this->expect_index] == false)
        {
            this->buffer[i+index-this->expect_index] = data[i];
            this->arrived[i+index-this->expect_index] = true;
            this->_unassembled_bytes ++;
        }
    }
    if(eof)
    {
        this->_eof = index + data.length();
    }
    this->try_output();
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes; }

bool StreamReassembler::empty() const { return _unassembled_bytes == 0; }
