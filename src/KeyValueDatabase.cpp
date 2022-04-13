#include "KeyValueDatabase.h"
#include "leveldb/db.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


namespace keyvaluedatabase
{

class KeyValueDatabaseImpl : public KeyValueDatabase
{
public:
	KeyValueDatabaseImpl(const char *databaseLocation)
	{
		leveldb::Options options;
		options.create_if_missing = true;
		leveldb::Status status = leveldb::DB::Open(options, databaseLocation, &mDatabase);
		if ( !status.ok() )
		{
			printf("Failed to open database(%s)\n", databaseLocation);
			delete mDatabase;
			mDatabase = nullptr;
		}
	}

	virtual ~KeyValueDatabaseImpl(void)
	{
		delete mIterator;
		delete mDatabase;
	}


	// Start iterating the database starting with a key that matches this prefix. An empty string or null pointer will start at the first key in the database
	virtual bool begin(const char *prefix) final
	{
		bool ret = false;

		if ( mDatabase )
		{
			delete mIterator;
			mIterator = mDatabase->NewIterator(leveldb::ReadOptions());
			mPrefix.clear();
			if ( prefix )
			{
				mPrefix = std::string(prefix);
			}
			if ( mPrefix.size() )
			{
				mIterator->Seek(mPrefix);
			}
			else
			{
				mIterator->SeekToFirst();
			}
			if ( mIterator->Valid() )
			{
				ret = true;
			}
			else
			{
				delete mIterator;
				mIterator = nullptr;
			}
		}

		return ret;
	}

	// Returns the next key value pair if it matches the prefix
	virtual bool next(std::string &key,std::string &value) final
	{
		bool ret = false;

		if ( mIterator && mIterator->Valid() )
		{
			key = mIterator->key().ToString();
			value = mIterator->value().ToString();
			ret = true;
			mIterator->Next();
			if ( mIterator->Valid() && mPrefix.size() )
			{
				std::string skey = mIterator->key().ToString();
				if ( !strncmp(mPrefix.c_str(),skey.c_str(),mPrefix.size()) == 0 )
				{
					delete mIterator;
					mIterator = nullptr;
					ret = false;
				}
			}
		}

		return ret;
	}


	virtual void release(void) final
	{
		delete this;
	}

	bool isValid(void) const
	{
		return mDatabase ? true :false;
	}

	std::string			mPrefix;
	leveldb::DB 		*mDatabase{nullptr};
	leveldb::Iterator	*mIterator{nullptr};
};

KeyValueDatabase *KeyValueDatabase::create(const char *databaseLocation)
{
	auto ret = new KeyValueDatabaseImpl(databaseLocation);
	if ( !ret->isValid() )
	{
		delete ret;
		ret = nullptr;
	}
	return static_cast< KeyValueDatabase *>(ret);
}


}
