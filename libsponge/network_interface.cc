#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

bool NetworkInterface::_has_valid_addr(const uint32_t ip_addr)
{
    auto p = _arp_table.find(ip_addr);
    if(p == _arp_table.end())
        return false;
    if(p->second._expire_time <= _time)
        return false;
    return true;
}

bool NetworkInterface::_send(const uint32_t ip_addr)
{
    if(!_has_valid_addr(ip_addr))
        return false;
    EthernetAddress addr = _arp_table[ip_addr].address;
    while(_queued_frame[ip_addr].size() > 0)
    {
        auto datagram = _queued_frame[ip_addr].front();
        EthernetFrame frame;
        frame.header().dst = addr;
        frame.header().src = _ethernet_address;
        frame.header().type = EthernetHeader::TYPE_IPv4;
        frame.payload() = datagram.serialize();
        _frames_out.push(frame);
        _queued_frame[ip_addr].pop();
    }
    return true;
}

void NetworkInterface::_send_arp_request(const uint32_t ip_addr)
{
    auto p = _arp_table.find(ip_addr);
    if(p != _arp_table.end() && p->second._require_time + 5000 > _time)
        return;
    _arp_table[ip_addr]._require_time = _time;

    ARPMessage msg;
    msg.opcode = ARPMessage::OPCODE_REQUEST;
    msg.sender_ethernet_address = _ethernet_address;
    msg.sender_ip_address = _ip_address.ipv4_numeric();
    msg.target_ip_address = ip_addr;

    EthernetFrame frame;
    frame.header().dst = ETHERNET_BROADCAST;
    frame.header().src = _ethernet_address;
    frame.header().type = EthernetHeader::TYPE_ARP;
    frame.payload() = msg.serialize();
    _frames_out.push(frame);
}

void NetworkInterface::_send_arp_reply(const EthernetAddress& eth_addr, const uint32_t ip_addr)
{
    ARPMessage msg;
    msg.opcode = ARPMessage::OPCODE_REPLY;
    msg.sender_ethernet_address = _ethernet_address;
    msg.sender_ip_address = _ip_address.ipv4_numeric();
    msg.target_ethernet_address = eth_addr;
    msg.target_ip_address = ip_addr;

    EthernetFrame frame;
    frame.header().dst = eth_addr;
    frame.header().src = _ethernet_address;
    frame.header().type = EthernetHeader::TYPE_ARP;
    frame.payload() = msg.serialize();
    _frames_out.push(frame);
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();
    _queued_frame[next_hop_ip].push(dgram);
    if(!_send(next_hop_ip))
    {
        _send_arp_request(next_hop_ip);
    }
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    if (frame.header().dst != _ethernet_address && frame.header().dst != ETHERNET_BROADCAST) 
    {
        return nullopt;
    }
    if(frame.header().type == EthernetHeader::TYPE_IPv4)
    {
        InternetDatagram datagram;
        auto result = datagram.parse(frame.payload());
        if(result == ParseResult::NoError)
        {
            return datagram;
        }
        else
        {
            return nullopt;
        }
    }
    if(frame.header().type == EthernetHeader::TYPE_ARP)
    {
        ARPMessage msg;
        auto result = msg.parse(frame.payload());
        if(result != ParseResult::NoError)
        {
            return nullopt;
        }
        if(msg.opcode == ARPMessage::OPCODE_REQUEST && msg.target_ip_address == _ip_address.ipv4_numeric())
        {
            _send_arp_reply(msg.sender_ethernet_address, msg.sender_ip_address);
            arp_table_item item = {msg.sender_ethernet_address, _time + 30*1000, _time - 5*1000};
            _arp_table[msg.sender_ip_address] = item;
            _send(msg.sender_ip_address);
        }
        if(msg.opcode == ARPMessage::OPCODE_REPLY)
        {
            arp_table_item item = {msg.sender_ethernet_address, _time + 30*1000, _time - 5*1000};
            _arp_table[msg.sender_ip_address] = item;
            _send(msg.sender_ip_address);
        }
    }
    return nullopt;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
    _time += ms_since_last_tick;
    // for(auto arp_item = _arp_table.begin(); arp_item != _arp_table.end(); )
    // {
    //     if(arp_item->second._expire_time > _time)
    //         arp_item = _arp_table.erase(arp_item);
    //     else
    //         arp_item ++;
    // }
 }
