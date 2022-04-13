#pragma once

#include <string>

namespace keyvaluedatabase
{

class KeyValueDatabase
{
public:
	static KeyValueDatabase *create(const char *databaseLocation);

	// Start iterating the database starting with a key that matches this prefix. An empty string or null pointer will start at the first key in the database
	virtual bool begin(const char *prefix) = 0;

	// Returns the next key value pair if it matches the prefix
	virtual bool next(std::string &key,std::string &value) = 0;

	virtual void release(void) = 0;
protected:
	virtual ~KeyValueDatabase(void)
	{
	}
};

}
