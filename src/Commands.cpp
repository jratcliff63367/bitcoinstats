#include "Commands.h"
#include "GetArgs.h"
#include "sutil.h"
#include "StandardDeviation.h"
#include "rand.h"
#include "Blocks.h"
#include "ParseBlock.h"
#include "CBlockIndex.h"

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
	CommandsImpl(uint32_t argc,const char **argv)
	{
		if ( argc )
		{
			mDataDir = argv[0];
			mIndexDir = mDataDir + "/blocks/index";
			mBlocksDir = mDataDir + "/blocks";
		}

		mCommands["help"] = CommandType::help;
		mCommands["bye"] = CommandType::bye;
		mCommands["block"] = CommandType::block;

		printf("Enter a command. Type 'help' for help. Type 'bye' to exit.\n");

		mBlocks = blocks::Blocks::create(mIndexDir.c_str());

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
					printf("bye        : Exit application\n");
					printf("block <n>  : Parse bitcoin block at this block height\n");
					break;
				case CommandType::block:
					if ( argc >= 2 )
					{
						int block = atoi(argv[1]);
						if (block >= 0 && block < (int)mBlocks->getBlockHeight() )
						{
							const CBlockIndex *cbi = mBlocks->getBlockIndex(block);
							if ( cbi )
							{
								parseblock::ParseBlock *pb = parseblock::ParseBlock::create();
								pb->parseBlock(*cbi,mBlocksDir.c_str());
								pb->release();
							}
							else
							{
								printf("Failed to find block height:%s\n", argv[1]);
							}
						}
						else
						{
							printf("Invalid block height:%s\n", argv[1]);
						}
					}
					else
					{
						printf("Usage: block <blockNumber>\n");
					}
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
	std::string		mDataDir;
	std::string		mIndexDir;
	std::string		mBlocksDir;
};

Commands *Commands::create(uint32_t argc,const char **argv)
{
	auto ret = new CommandsImpl(argc,argv);
	return static_cast< Commands *>(ret);
}


}

