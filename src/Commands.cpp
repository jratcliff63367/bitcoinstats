#include "Commands.h"
#include "GetArgs.h"
#include "sutil.h"
#include "StandardDeviation.h"
#include "rand.h"
#include "Blocks.h"

#include <stdio.h>
#include <stdlib.h>
#include <iostream> 
#include <string> 
#include <assert.h>
#include <math.h>
#include <float.h>

#include <unordered_map>

#pragma warning(disable:4100)

#define SAFE_RELEASE(x) if ( x ) { x->release(); x = nullptr; }

namespace commands
{

static Rand rand;


double randomRange(double min,double max)
{
	return (rand.Ranf()*(max-min))+min;
}

using CommandTypeMap = std::unordered_map< std::string, CommandType >;

class CommandsImpl : public Commands
{
public:
	CommandsImpl(void)
	{
		mCommands["help"] = CommandType::help;
		mCommands["bye"] = CommandType::bye;

		printf("Enter a command. Type 'help' for help. Type 'bye' to exit.\n");

		mBlocks = blocks::Blocks::create("d:\\bitcoin-data\\blocks\\index");

	}

	virtual ~CommandsImpl(void)
	{
		SAFE_RELEASE(mBlocks);
	}

	virtual bool processInput(const char *str) final
	{
		bool ret = false;


		getargs::GetArgs g;
		uint64_t argc;
		const char **argv = g.getArgs(str,argc);
		if ( argv )
		{
			ret = processCommand(argc,argv);
		}


		return ret;
	}

	bool processCommand(uint64_t argc,const char **argv)
	{
		bool ret = false;

		if ( argc >= 1 )
		{
			CommandType c = getCommandType(argv[0]);
			switch ( c )
			{
				case CommandType::bye:
					ret = true;
					break;
				case CommandType::help:
					printf("There is no help for you.\n");
					break;
				case CommandType::last:
					printf("Unknown command: %s\n", argv[0]);
					break;
				default:
					printf("Command: %s not yet implemented.\n", argv[0]);
					break;
			}
		}

		return ret;
	}

	virtual void release(void) final
	{
		delete this;
	}

	virtual CommandType getCommandType(const char *str) const final
	{
		CommandType ret = CommandType::last;

		CommandTypeMap::const_iterator found = mCommands.find(std::string(str));
		if ( found != mCommands.end() )
		{
			ret = (*found).second;
		}

		return ret;
	}

	bool			mExit{false};
	CommandTypeMap mCommands;
	blocks::Blocks	*mBlocks{nullptr};
};

Commands *Commands::create(void)
{
	auto ret = new CommandsImpl;
	return static_cast< Commands *>(ret);
}


}

