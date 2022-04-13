#pragma once

#include <stdint.h>

namespace commands
{

enum class CommandType : uint32_t
{
	help,
	bye,
	last
};

class Commands
{
public:
	static Commands *create(void);

	virtual bool processInput(const char *inputLine) = 0;

	virtual CommandType getCommandType(const char *str) const = 0;

	virtual void release(void) = 0;
protected:
	virtual ~Commands(void)
	{
	} 

};

}
