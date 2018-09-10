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
#ifndef SRC_STRINGCACHE_H_
#define SRC_STRINGCACHE_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

namespace aiengine {

class StringCache {
public:
    	explicit StringCache(const char *value);
    	explicit StringCache():StringCache("") {}
    	virtual ~StringCache() { value_.clear();}

#ifdef HAVE_STATIC_MEMORY_CACHE
	static const int max_static_memory = 256;
#endif
	void reset(); 
	const char *getName() const { return value_.c_str(); }
	void setName(const char *name, int length); 
	void setName(const char *name);

	size_t getNameSize() const { return value_.size(); }

#ifdef PYTHON_BINDING
	friend std::ostream& operator<< (std::ostream &out, const StringCache &sc) {
	
		out << sc.value_;
        	return out;
	}
#endif

private:
	std::string value_;
};

} // namespace aiengine  

#endif  // SRC_STRINGCACHE_H_