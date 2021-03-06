/*
 * AIEngine a new generation network intrusion detection system.
 *
 * Copyright (C) 2013-2018  Luis Campo Giralte
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Ryadnology Team; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Ryadnology Team, 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 * Written by Luis Campo Giralte <me@ryadpasha.com> 
 *
 */
#include "test_ip.h"

#define BOOST_TEST_DYN_LINK
#ifdef STAND_ALONE_TEST
#define BOOST_TEST_MODULE iptest
#endif

#include <boost/test/unit_test.hpp>

using namespace aiengine;

BOOST_FIXTURE_TEST_SUITE(ip_test_suite, StackEthernetIP)

BOOST_AUTO_TEST_CASE (test01)
{
	BOOST_CHECK(ip->getTotalPackets() == 0);
	BOOST_CHECK(ip->getTotalBytes() == 0);
	BOOST_CHECK(ip->getTotalValidPackets() == 0);
	BOOST_CHECK(ip->getTotalInvalidPackets() == 0);

	ip->processFlow(nullptr); // Nothing to do
}
	
BOOST_AUTO_TEST_CASE (test02)
{
	std::string localip("192.168.122.127");	
	std::string remoteip("54.243.216.57");	

	Packet packet("../tcp/packets/packet01.pcap");

	const uint8_t *raw_packet = packet.getPayload();

	ip->setHeader(&raw_packet[14]);
	BOOST_CHECK(ip->getTotalPackets() == 0);
	BOOST_CHECK(ip->getTTL() == 64);
	BOOST_CHECK(ip->getIPHeaderLength() == 20);
	BOOST_CHECK(ip->getProtocol() == IPPROTO_TCP);
	BOOST_CHECK(ip->getPacketLength() == packet.getLength() - 14);
	BOOST_CHECK(ip->isFragment() == false);

	BOOST_CHECK(localip.compare(ip->getSrcAddrDotNotation())==0);
	BOOST_CHECK(remoteip.compare(ip->getDstAddrDotNotation())==0);
}

BOOST_AUTO_TEST_CASE (test03) // Invalid IP packet
{
	Packet packet("../dns/packets/packet34.pcap");
	packet.setPayloadLength(18);

	inject(packet);

	BOOST_CHECK(ip->getTotalPackets() == 0);
	BOOST_CHECK(ip->getTotalValidPackets() == 0);
	BOOST_CHECK(ip->getTotalInvalidPackets() == 1);
	BOOST_CHECK(ip->getTotalBytes() == 0);
}

BOOST_AUTO_TEST_CASE (test04) // ethernet -> ip 
{
	Packet packet("../dns/packets/packet34.pcap");
	
	mux_eth->setPacket(&packet);
	eth->setHeader(packet.getPayload());     
	// Sets the raw packet to a valid ethernet header
        BOOST_CHECK(eth->getEthernetType() == ETHERTYPE_IP);

	inject(packet);

        BOOST_CHECK(mux_eth->getCurrentPacket()->getLength() == packet.getLength());

	BOOST_CHECK(ip->getTotalPackets() == 1);
	BOOST_CHECK(ip->getTotalValidPackets() == 1);
	BOOST_CHECK(ip->getTotalInvalidPackets() == 0);
	BOOST_CHECK(ip->getTotalBytes() == packet.getLength() - 14);
	BOOST_CHECK(ip->isFragment() == false);
	BOOST_CHECK(ip->getTotalEvents() == 0);
}

BOOST_FIXTURE_TEST_CASE (test05, StackEthernetVLanIP) // ethernet -> vlan -> ip 
{
	Packet packet("../dns/packets/packet35.pcap");

	inject(packet);

	BOOST_CHECK(vlan->getTotalPackets() == 1);
	BOOST_CHECK(ip->getTotalPackets() == 1);

        BOOST_CHECK(mux_eth->getTotalForwardPackets() == 1);
        BOOST_CHECK(mux_vlan->getTotalForwardPackets() == 1);
        BOOST_CHECK(mux_ip->getTotalForwardPackets() == 0);
        BOOST_CHECK(mux_vlan->getTotalFailPackets() == 0);

	BOOST_CHECK(vlan->getTotalBytes() == packet.getLength() - 14);
	BOOST_CHECK(ip->getTotalBytes() == packet.getLength() - 18);
	BOOST_CHECK(ip->isFragment() == false);
        BOOST_CHECK(mux_eth->getCurrentPacket()->getLength() == packet.getLength());

        BOOST_CHECK(eth->getEthernetType() == ETHERTYPE_VLAN);
        BOOST_CHECK(vlan->getEthernetType() == ETHERTYPE_IP);
}

BOOST_FIXTURE_TEST_CASE (test06, StackEthernetVLanIP) // ethernet -> vlan -> ip
{
	Packet packet("../dns/packets/packet34.pcap");

	inject(packet);
        
	BOOST_CHECK(eth->getTotalPackets() == 0);
	BOOST_CHECK(vlan->getTotalPackets() == 0);
        BOOST_CHECK(ip->getTotalPackets() == 0);

        BOOST_CHECK(mux_eth->getTotalForwardPackets() == 0);
        BOOST_CHECK(mux_eth->getTotalFailPackets() == 1);
        BOOST_CHECK(mux_vlan->getTotalForwardPackets() == 0);
        BOOST_CHECK(mux_vlan->getTotalFailPackets() == 0);
        BOOST_CHECK(mux_ip->getTotalForwardPackets() == 0);
        BOOST_CHECK(mux_ip->getTotalFailPackets() == 0);

        BOOST_CHECK(mux_eth->getCurrentPacket()->getLength() == packet.getLength());
        BOOST_CHECK(mux_vlan->getCurrentPacket()->getLength() == 0);
        BOOST_CHECK(mux_ip->getCurrentPacket()->getLength() == 0);
}

BOOST_AUTO_TEST_CASE (test07) // ethernet -> vlan -> ip
{
        VLanProtocol *vlan = new VLanProtocol();
        MultiplexerPtr mux_vlan = MultiplexerPtr(new Multiplexer());
        IPProtocol *ip1 = new IPProtocol();
        MultiplexerPtr mux_ip1 = MultiplexerPtr(new Multiplexer());
        IPProtocol *ip2 = new IPProtocol();
        MultiplexerPtr mux_ip2 = MultiplexerPtr(new Multiplexer());

	Packet packet("../ip/packets/packet03.pcap");

	inject(packet);

        // configure the vlan handler
        vlan->setMultiplexer(mux_vlan);
        mux_vlan->setProtocol(static_cast<ProtocolPtr>(vlan));
        mux_vlan->setProtocolIdentifier(ETHERTYPE_VLAN);
        mux_vlan->setHeaderSize(vlan->getHeaderSize());
        mux_vlan->addChecker(std::bind(&VLanProtocol::vlanChecker, vlan, std::placeholders::_1));
        mux_vlan->addPacketFunction(std::bind(&VLanProtocol::processPacket, vlan, std::placeholders::_1));

        // configure the ip1
        ip1->setMultiplexer(mux_ip1);
        mux_ip1->setProtocolIdentifier(ETHERTYPE_IP);
        mux_ip1->setProtocol(static_cast<ProtocolPtr>(ip1));
        mux_ip1->setHeaderSize(ip1->getHeaderSize());
        mux_ip1->addChecker(std::bind(&IPProtocol::ipChecker, ip1, std::placeholders::_1));
        mux_ip1->addPacketFunction(std::bind(&IPProtocol::processPacket, ip1, std::placeholders::_1));

        // configure the ip1
        ip2->setMultiplexer(mux_ip2);
        mux_ip2->setProtocolIdentifier(IPPROTO_IPIP);
        mux_ip2->setProtocol(static_cast<ProtocolPtr>(ip2));
        mux_ip2->setHeaderSize(ip2->getHeaderSize());
        mux_ip2->addChecker(std::bind(&IPProtocol::ipChecker, ip2, std::placeholders::_1));
        mux_ip2->addPacketFunction(std::bind(&IPProtocol::processPacket, ip2, std::placeholders::_1));

        // configure the multiplexers
       	mux_eth->addUpMultiplexer(mux_vlan, ETHERTYPE_VLAN);
        mux_vlan->addDownMultiplexer(mux_eth);
        mux_eth->addUpMultiplexer(mux_ip1, ETHERTYPE_IP);
        mux_ip1->addDownMultiplexer(mux_eth);
	mux_ip1->addUpMultiplexer(mux_ip2, IPPROTO_IPIP);
	mux_ip2->addDownMultiplexer(mux_ip1);

	inject(packet);

	// Now check all the path that the packet have take
        BOOST_CHECK(vlan->getTotalPackets() == 0);
        BOOST_CHECK(ip1->getTotalInvalidPackets() == 0);
        BOOST_CHECK(ip2->getTotalInvalidPackets() == 0);

        BOOST_CHECK(mux_vlan->getTotalForwardPackets() == 0);
        BOOST_CHECK(mux_ip1->getTotalForwardPackets() == 1);
        BOOST_CHECK(mux_ip2->getTotalForwardPackets() == 0);
        BOOST_CHECK(mux_ip2->getTotalFailPackets() == 1);

	// check the integrity of the ethernet
        BOOST_CHECK(eth->getEthernetType() == ETHERTYPE_IP);

	// check the integrity of the first ip header
        BOOST_CHECK(ip1->getTTL() == 64);
        BOOST_CHECK(ip1->getIPHeaderLength() == 20);
        BOOST_CHECK(ip1->getProtocol() == IPPROTO_IPIP);
        BOOST_CHECK(ip1->getPacketLength() == packet.getLength() - 14);

        BOOST_CHECK(mux_eth->getCurrentPacket()->getLength() == packet.getLength());
        BOOST_CHECK(mux_vlan->getCurrentPacket()->getLength() == 0);

	BOOST_CHECK(ip1->isFragment() == false);
	BOOST_CHECK(ip2->isFragment() == false);
        BOOST_CHECK(ip1->getPacketLength() == 81);
        BOOST_CHECK(ip2->getPacketLength() == 61);

        BOOST_CHECK(ip1->getProtocol() == IPPROTO_IPIP);
        BOOST_CHECK(ip2->getProtocol() == IPPROTO_UDP);

	// check integrity of the second ip header
	std::string src_ip("192.168.1.118");
	std::string dst_ip("80.58.61.250");

        BOOST_CHECK(src_ip.compare(ip2->getSrcAddrDotNotation())==0);
        BOOST_CHECK(dst_ip.compare(ip2->getDstAddrDotNotation())==0);
}

BOOST_AUTO_TEST_SUITE_END( )

BOOST_FIXTURE_TEST_SUITE(ip_frag_test_suite, StackEthernetIP)

// First packet of a fragmented IP packet
BOOST_AUTO_TEST_CASE (test01)
{
	Packet packet("../ip/packets/packet01.pcap");

	inject(packet);

        BOOST_CHECK(mux_eth->getCurrentPacket()->getLength() == packet.getLength());

        BOOST_CHECK(ip->getTotalPackets() == 1);
        BOOST_CHECK(ip->getTotalValidPackets() == 1);
        BOOST_CHECK(ip->getTotalInvalidPackets() == 0);
        BOOST_CHECK(ip->getTotalBytes() == packet.getLength() - 14);

	BOOST_CHECK(ip->getPacketLength() == 1500);
	BOOST_CHECK(ip->getID() == 29812);
	BOOST_CHECK(ip->isFragment() == true);
	BOOST_CHECK(ip->getTotalEvents() == 1);
	BOOST_CHECK(ip->getProtocol() == IPPROTO_ICMP);
}

// last packet of a fragmented IP packet
BOOST_AUTO_TEST_CASE (test02)
{
	Packet packet("../ip/packets/packet02.pcap");

	inject(packet);

        BOOST_CHECK(mux_eth->getCurrentPacket()->getLength() == packet.getLength());

        BOOST_CHECK(ip->getTotalPackets() == 1);
        BOOST_CHECK(ip->getTotalValidPackets() == 1);
        BOOST_CHECK(ip->getTotalInvalidPackets() == 0);
        BOOST_CHECK(ip->getTotalBytes() == packet.getLength() - 14);

        BOOST_CHECK(ip->getPacketLength() == 568);
        BOOST_CHECK(ip->getID() == 29812);
        BOOST_CHECK(ip->isFragment() == true);
	BOOST_CHECK(ip->getProtocol() == IPPROTO_ICMP);

	// For exercise the code
	CounterMap c = ip->getCounters();
}

BOOST_AUTO_TEST_SUITE_END( )

