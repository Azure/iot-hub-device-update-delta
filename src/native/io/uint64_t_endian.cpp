#ifndef WIN32

#include "uint64_t_endian.h"

#include <arpa/inet.h>

uint64_t ntohll(uint64_t value)
{
    // Check if the system is little-endian
    static const int num = 1;
    if (*(const char *)&num == 1)
    {
        // Convert from network byte order to host byte order
        uint32_t high = ntohl(static_cast<uint32_t>(value >> 32));
        uint32_t low = ntohl(static_cast<uint32_t>(value & 0xFFFFFFFF));
        return (static_cast<uint64_t>(low) << 32) | high;
    }
    else
    {
        // System is big-endian, no conversion needed
        return value;
    }
}

uint64_t htonll(uint64_t value)
{
	// Check if the system is little-endian
	static const int num = 1;
	if (*(const char *)&num == 1)
	{
		// Convert from network byte order to host byte order
		uint32_t high = htonl(static_cast<uint32_t>(value >> 32));
		uint32_t low  = htonl(static_cast<uint32_t>(value & 0xFFFFFFFF));
		return (static_cast<uint64_t>(low) << 32) | high;
	}
	else
	{
		// System is big-endian, no conversion needed
		return value;
	}
}

#endif