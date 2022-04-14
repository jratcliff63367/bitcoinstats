#pragma once

#include <stdint.h>

/**
* reads a variable length integer.
* See the documentation from here:  https://en.bitcoin.it/wiki/Protocol_specification#Variable_length_integer
* 
* @param data : The pointer to the variable length integer
* @param value : A reference to a 64 bit integer to return the decompressed value
* 
* @return : Returns the address in memory immediately after reading the variable length integer
*/
inline const void *readVariableLengthInteger(const void *data,uint64_t &value)
{
	const uint8_t *ret = (const uint8_t *)data;	// Cast the void pointer to a single byte pointer

	uint8_t v8 = *ret; // Get the first byte of data in the variable length integer
	if (v8 < 0xFD) // If it's less than 0xFD use this value as the unsigned integer
	{
		value = (uint64_t)v8; // Cast the result as a 64 bit integer
		ret++;	// Increment the pointer by just one byte
	}
	else
	{
		uint16_t v16 = *(const uint16_t *)ret;	// Get the value as a 16 bit integer
		if (v16 < 0xFFFF)	// If it is less than 0xFFFF we just return the 16 bit result
		{
			value = (uint64_t)v16; // Cast the 16 bit integer to a 64 bit integer
			ret+=2;	// Skip the 2 bytes for the 16 bit integer value
		}
		else
		{
			uint32_t v32 = *(const uint32_t *)ret;	// Get the value as a 32 bit integer
			if (v32 < 0xFFFFFFFF)	// If the value is less than 0xFFFFFFFF we just return the 32 bit value
			{
				value = (uint64_t)v32; // cast the 32 bit value as 64 bit
				ret+=4;	// Skip 4 bytes in the return
			}
			else
			{
				value = *(const uint64_t *)ret; // Retrieve the full 64 bit integer
				ret+=8;	// Skip the 8 bytes of the 64 bit integer
			}
		}
	}
	return ret; // Return the new pointer location
}