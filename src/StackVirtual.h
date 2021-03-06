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
 * Configuration diagram of the stack
 *
 *                         +--------------------+
 *                         | TCPGenericProtocol |                    
 *                         +-------+------------+                    
 *                                 |                                 
 *          +--------------------+ |              +--------------------+
 *          |     SSLProtocol    | |              | UDPGenericProtocol |
 *          +--------------+-----+ |              +-----------+--------+
 *                         |       |                          |      
 * +--------------------+  |       |  +--------------------+  |      
 * |    HTTPProtocol    |  |       |  |    DNSProtocol     |  |      
 * +------------------+-+  |       |  +------------+-------+  |      
 *                    |    |       |               |          |      
 *                 +--+----+-------+----+    +-----+----------+---+  
 *                 |    TCPProtocol     |    |    UDPProtocol     |  
 *                 +------------------+-+    +-+------------------+  
 *                                    |        |                     
 *      +--------------------+        |        |                     
 *      |   ICMPProtocol     +-----+  |        |                     
 *      +--------------------+     |  |        |                     
 *                               +-+--+--------+------+              
 *                               |     IPProtocol     |              
 *                               +---------+----------+              
 *                                         |                         
 *                               +---------+----------+              
 *                               |  EthernetProtocol  +--+           
 *                               +--+-----------------+  |           
 *                                  |                    |           
 *                                  |          +---------+----------+
 *                                  |          |   VxLanProtocol    |
 *                                  |          +---------+----------+
 *                                  |                    |           
 *                +-----------------+--+       +---------+----------+
 *                |    GREProtocol     |       |    UDPProtocol     |
 *                +-----------------+--+       +--+-----------------+
 *                                  |             |                  
 *                               +--+-------------+---+              
 *                         +---> |     IPProtocol     | <---+        
 *                         |     +---------+----------+     |        
 *                         |               |                |        
 *                +--------+-----------+   |   +------------+-------+
 *                |    VLANProtocol    |   |   |    MPLSProtocol    |
 *                +--------+-----------+   |   +------------+-------+
 *                         |               |                |        
 *                         |     +---------+----------+     |        
 *                         +-----+  EthernetProtocol  +-----+        
 *                               +--------------------+              
 *
 */
#ifndef SRC_STACKVIRTUAL_H_
#define SRC_STACKVIRTUAL_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <chrono>
#include <string>
#ifdef HAVE_LIBLOG4CXX
#include "log4cxx/logger.h"
#endif
#include "Multiplexer.h"
#include "FlowForwarder.h"
#include "protocols/ip/IPProtocol.h"
#include "protocols/vxlan/VxLanProtocol.h"
#include "protocols/gre/GREProtocol.h"
#include "protocols/udp/UDPProtocol.h"
#include "protocols/tcp/TCPProtocol.h"
#include "protocols/icmp/ICMPProtocol.h"
#include "flow/FlowManager.h"
#include "flow/FlowCache.h"
#include "NetworkStack.h"
#include "DatabaseAdaptor.h"

namespace aiengine {

class StackVirtual: public NetworkStack {
public:
	explicit StackVirtual();
        virtual ~StackVirtual() {}

        MultiplexerPtrWeak getLinkLayerMultiplexer() override { return mux_eth;}

        void statistics(std::basic_ostream<char> &out) const override;
	
	void showFlows(std::basic_ostream<char> &out, int limit) override;
	void showFlows(std::basic_ostream<char> &out, const std::string &protoname, int limit) override;

	void setTotalTCPFlows(int value) override;
	void setTotalUDPFlows(int value) override;
        int getTotalTCPFlows() const override;
        int getTotalUDPFlows() const override;

	void enableNIDSEngine(bool enable) override;
	void enableFrequencyEngine(bool enable) override;
        bool isEnableFrequencyEngine() const override { return enable_frequency_engine_; }
        bool isEnableNIDSEngine() const override { return enable_nids_engine_; }

	void setFlowsTimeout(int timeout) override;
	int getFlowsTimeout() const override { return flow_table_tcp_vir_->getTimeout(); }

#if defined(BINDING)
        FlowManager &getTCPFlowManager() override { return *flow_table_tcp_vir_.get(); }
        FlowManager &getUDPFlowManager() override { return *flow_table_udp_vir_.get(); }
#else
        FlowManagerPtrWeak getTCPFlowManager() override { return flow_table_tcp_vir_; }
        FlowManagerPtrWeak getUDPFlowManager() override { return flow_table_udp_vir_; }
#endif

	void setTCPRegexManager(const SharedPointer<RegexManager> &rm) override;
        void setUDPRegexManager(const SharedPointer<RegexManager> &rm) override;

        void setTCPIPSetManager(const SharedPointer<IPSetManager> &ipset_mng) override;
        void setUDPIPSetManager(const SharedPointer<IPSetManager> &ipset_mng) override;

#if defined(RUBY_BINDING) || defined(LUA_BINDING) || defined(GO_BINDING) 
        void setTCPRegexManager(RegexManager &rm) { setTCPRegexManager(std::make_shared<RegexManager>(rm)); }
        void setUDPRegexManager(RegexManager &rm) { setUDPRegexManager(std::make_shared<RegexManager>(rm)); }

        void setTCPIPSetManager(IPSetManager &ipset_mng) { setTCPIPSetManager(std::make_shared<IPSetManager>(ipset_mng)); }
        void setUDPIPSetManager(IPSetManager &ipset_mng) { setUDPIPSetManager(std::make_shared<IPSetManager>(ipset_mng)); }
#elif defined(JAVA_BINDING)
        void setTCPRegexManager(RegexManager *sig);
        void setUDPRegexManager(RegexManager *sig);

        void setTCPIPSetManager(IPSetManager *ipset_mng);
        void setUDPIPSetManager(IPSetManager *ipset_mng);
#endif
	std::tuple<Flow*,Flow*> getCurrentFlows() const override; 

private:
	typedef NetworkStack super_;
#ifdef HAVE_LIBLOG4CXX
	static log4cxx::LoggerPtr logger;
#endif
        //Protocols
	IPProtocolPtr ip_;
        UDPProtocolPtr udp_;
	VxLanProtocolPtr vxlan_;
	GREProtocolPtr gre_;
	EthernetProtocolPtr eth_vir_;
	IPProtocolPtr ip_vir_;
	UDPProtocolPtr udp_vir_;
        TCPProtocolPtr tcp_vir_;
        ICMPProtocolPtr icmp_;

        // Specific Multiplexers
        MultiplexerPtr mux_udp_;
        MultiplexerPtr mux_vxlan_;
        MultiplexerPtr mux_gre_;
        MultiplexerPtr mux_eth_vir_;
        MultiplexerPtr mux_ip_vir_;
        MultiplexerPtr mux_udp_vir_;
        MultiplexerPtr mux_tcp_vir_;
        MultiplexerPtr mux_icmp_;

        // FlowManager and FlowCache
	FlowManagerPtr flow_table_udp_;
        FlowManagerPtr flow_table_udp_vir_;
        FlowManagerPtr flow_table_tcp_vir_;
        FlowCachePtr flow_cache_udp_;
        FlowCachePtr flow_cache_udp_vir_;
        FlowCachePtr flow_cache_tcp_vir_;

        // FlowForwarders
	SharedPointer<FlowForwarder> ff_vxlan_;
	SharedPointer<FlowForwarder> ff_udp_;
        SharedPointer<FlowForwarder> ff_tcp_vir_;
        SharedPointer<FlowForwarder> ff_udp_vir_;

        bool enable_frequency_engine_;
        bool enable_nids_engine_;
};

typedef std::shared_ptr<StackVirtual> StackVirtualPtr;

} // namespace aiengine

#endif  // SRC_STACKVIRTUAL_H_
