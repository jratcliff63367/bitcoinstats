#include "blocks.h"
#include "varint.h"
#include "KeyValueDatabase.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "CBlockIndex.h"

#include <assert.h>

#ifdef _MSC_VER
#pragma warning(disable:4100)
#endif

namespace blocks
{

class BlocksImpl : public Blocks
{
public:
	BlocksImpl(const char *levelDBDir)
	{
		auto database = keyvaluedatabase::KeyValueDatabase::create(levelDBDir);
		if ( database )
		{
			bool ok = database->begin("b");
			while (ok )
			{
				std::string key;
				std::string value;
				ok = database->next(key,value);
				if ( ok )
				{
					CBlockIndex cb;
					const uint8_t *start = (const uint8_t *)value.c_str();
					printf("BlockHash:%s\n", getHexReverse(key).c_str());
					printf("CBlockIndexData:%s\n", getHex(value).c_str());
					cb.readBlockIndex(start,value.size());
					cb.printInfo();
				}
			}
			database->release();
		}
	}
	virtual ~BlocksImpl(void)
	{
	}
	virtual void release(void) final
	{
		delete this;
	}

	char getHex(uint8_t c)
	{
		char ret;
		if ( c < 10 )
		{
			ret = '0'+c;
		}
		else
		{
			ret = 'A'+(c-10);
		}
		return ret;
	}

	std::string getHex(const std::string &str)
	{
		std::string ret;
		for (auto &i:str)
		{
			uint8_t c = uint8_t(i);
			char h1 = getHex(c>>4);
			char h2 = getHex(c&15);
			ret.push_back(h1);
			ret.push_back(h2);
		}
		return ret;
	}

	std::string getHexReverse(const std::string &str)
	{
		std::string ret;
		size_t count = str.size();
		size_t index = count-1;
		for (size_t i=0; i<(count-1); i++)
		{
			uint8_t c = uint8_t(str[index]);
			char h1 = getHex(c>>4);
			char h2 = getHex(c&15);
			ret.push_back(h1);
			ret.push_back(h2);
			index--;
		}
		return ret;
	}


};

Blocks *Blocks::create(const char *levelDBDir)
{
	auto ret = new BlocksImpl(levelDBDir);
	return static_cast< Blocks *>(ret);
}


}

