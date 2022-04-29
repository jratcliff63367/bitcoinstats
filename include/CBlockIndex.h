#pragma once

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <string>
#include <assert.h>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4100 4189) // disable these nag warnings on Visual Studio
#endif

/**
* This code snippet was written by John W. Ratcliff on April 14, 2022
* The purpose of this code snippet is to document the format of the
* bitcoin block headers which are written out to the leveldb database.
* 
* All code needed to parse one of these custom headers is located here,
* which includes the method to extract variable length integers.
* 
* *** IMPORTANT NOTE *** : The formatting of variable length integers by bitcoin core
* when it writes to the levelDB is *NOT* the same variable length format used by 
* the rest of the bitcoin blockchain. Moreover, the encoding is *NOT STANDARD*!
* It appears to be some custom routine rolled just for the bitcoin source code.
* So you must take a look at the 'readVarint128' method closely to see how
* these integers are decoded.
* 
* In general developers of bitcoin core consider this data to be specific to
* the bitcoin core code base; meaning they did not really expect independent
* parties to be reading it directly. That said, reading these block headers
* can *dramatically* speed up parsing the bitcoin blockchain. For people like
* myself who are interested primarily in data-mining the bitcoin blockchain
* this is extremely useful. The expected pattern is that you would read all
* of the bitcoin block index headers from LevelDB first, and then you would
* be able to directly seek to the correct file and offset locations to read 
* and parse that particular bitcoin block. Or, well, at least that is what
* I am intending on doing with it.
* 
* This is to serve as a more complete answer to the question previously
* posted here: https://bitcoin.stackexchange.com/questions/67515/format-of-a-block-keys-contents-in-bitcoinds-leveldb
* 
* The corresponding code in the bitcoin source code looks like this: https://github.com/bitcoin/bitcoin/blob/master/src/chain.h
*   SERIALIZE_METHODS(CDiskBlockIndex, obj)
    {
        int _nVersion = s.GetVersion();
        if (!(s.GetType() & SER_GETHASH)) READWRITE(VARINT_MODE(_nVersion, VarIntMode::NONNEGATIVE_SIGNED));

        READWRITE(VARINT_MODE(obj.nHeight, VarIntMode::NONNEGATIVE_SIGNED));
        READWRITE(VARINT(obj.nStatus));
        READWRITE(VARINT(obj.nTx));
        if (obj.nStatus & (BLOCK_HAVE_DATA | BLOCK_HAVE_UNDO)) READWRITE(VARINT_MODE(obj.nFile, VarIntMode::NONNEGATIVE_SIGNED));
        if (obj.nStatus & BLOCK_HAVE_DATA) READWRITE(VARINT(obj.nDataPos));
        if (obj.nStatus & BLOCK_HAVE_UNDO) READWRITE(VARINT(obj.nUndoPos));

        // block header
        READWRITE(obj.nVersion);
        READWRITE(obj.hashPrev);
        READWRITE(obj.hashMerkleRoot);
        READWRITE(obj.nTime);
        READWRITE(obj.nBits);
        READWRITE(obj.nNonce);
    }
*/

/**
* This class represents the decompressed / decoded contents of the leveldb block index data.
* On disk it is stored compressed and using some highly customized coding.
* This class will contain the decoded values.
*/
class CBlockIndex
{
public:
	uint64_t	mVersionNumber{0};	// version number encoded in the block header (stored in leveldb as a varible length integer) 
	uint64_t	mBlockHeight{0};	// Which block number this is (stored in leveldb as a varible length integer)
	uint64_t	mBlockStatus{0};	// Some bit flags about the block header (stored in leveldb as a variable length integer)
	uint64_t	mTransactionCount{0}; // Number of transactions in this block (stored in leveldb as a variable length integer)
	uint64_t	mFileIndex{0};		// blk or rev block number (stored in leveldb as a variable length integer)
	uint64_t	mFileOffset{0};		// The offset into this file where the block is located (stored in leveldb as a variable length integer)
	uint64_t	mUndoOffset{0};		// Offset into the rev file for undo data (stored in leveldb as a variable length integer)
	
	// These fields are the 'block header' data. These are not stored as variable length integers and are parsed directly
	// according to size of each value.
	int32_t		mBlockVersion{0};		// Version number
	uint8_t		mHashPrevious[32]{};	// The hash of the previous block
	uint8_t		mHashMerkleRoot[32]{};	// The hash of the merkle root
	uint32_t	mTime{0};				// The block timestamp in UNIX epoch time (time_t)
	uint32_t	mBits{0};				// This is the representation of the target; the value which the hash of the block header must not exceed in order to min the next block
	uint32_t	mNonce{0};				// This is a random number generated during the mining process

	uint8_t		mBlockHash[32]{};		// Hash of this block (key value when scanning leveldb)

	// This enumeration defines the possible states for the
	// bitcoin block status field
	enum BlockStatus: uint32_t 
	{
		//! Unused.
		BLOCK_VALID_UNKNOWN      =    0,
		//! Reserved (was BLOCK_VALID_HEADER).
		BLOCK_VALID_RESERVED     =    1,
		//! All parent headers found, difficulty matches, timestamp >= median previous, checkpoint. Implies all parents
		//! are also at least TREE.
		BLOCK_VALID_TREE         =    2,
		/**
		 * Only first tx is coinbase, 2 <= coinbase input script length <= 100, transactions valid, no duplicate txids,
		 * sigops, size, merkle root. Implies all parents are at least TREE but not necessarily TRANSACTIONS. When all
		 * parent blocks also have TRANSACTIONS, CBlockIndex::nChainTx will be set.
		 */
		BLOCK_VALID_TRANSACTIONS =    3,
		//! Outputs do not overspend inputs, no double spends, coinbase output ok, no immature coinbase spends, BIP30.
		//! Implies all parents are also at least CHAIN.
		BLOCK_VALID_CHAIN        =    4,
		//! Scripts & signatures ok. Implies all parents are also at least SCRIPTS.
		BLOCK_VALID_SCRIPTS      =    5,
		//! All validity bits.
		BLOCK_VALID_MASK         =   BLOCK_VALID_RESERVED | BLOCK_VALID_TREE | BLOCK_VALID_TRANSACTIONS |
									 BLOCK_VALID_CHAIN | BLOCK_VALID_SCRIPTS,
		BLOCK_HAVE_DATA          =    8, //!< full block available in blk*.dat
		BLOCK_HAVE_UNDO          =   16, //!< undo data available in rev*.dat
		BLOCK_HAVE_MASK          =   BLOCK_HAVE_DATA | BLOCK_HAVE_UNDO,
		BLOCK_FAILED_VALID       =   32, //!< stage after last reached validness failed
		BLOCK_FAILED_CHILD       =   64, //!< descends from failed block
		BLOCK_FAILED_MASK        =   BLOCK_FAILED_VALID | BLOCK_FAILED_CHILD,

		BLOCK_OPT_WITNESS        =   128, //!< block data in blk*.dat was received with a witness-enforcing client

		/**
		 * If set, this indicates that the block index entry is assumed-valid.
		 * Certain diagnostics will be skipped in e.g. CheckBlockIndex().
		 * It almost certainly means that the block's full validation is pending
		 * on a background chainstate. See `doc/assumeutxo.md`.
		 */
		BLOCK_ASSUMED_VALID      =   256,
	};


	CBlockIndex(void) { }; // default constructor

	// Optionally construct the class with the blockchash, which is found as the 'key' 
	// field in the levelDB databse. Important to note, this 'key' has a prefix of the letter 'b'
	// that you must skip past. It simply copies the blockhash provided into the data structure.
	CBlockIndex(const char *blockHash)
	{
		memcpy(mBlockHash,blockHash,sizeof(mBlockHash));
	}

	/**
	* Parse the leveldb block data and decompress it into the struct values
	* 
	* @param data : Contains the data loaded from leveldb
	* 
	* @return : Returns the pointer after this block has been parsed fully
	*/
	const void *readBlockIndex(const void *data,size_t expectedSize) // initialize the fields using this block data
	{
		const uint8_t *start = (const uint8_t *)data;

		// It is very important to note, that these values are *NOT* stored in the variable integer format used everywhere else
		// in the bitcoin blockchain. Instead, they are stored in a highly customized bitcoin core specific encoding.
		// I should add that there is absolutely no need to attempt to 'compress' this data!
		// LevelDB has a built-in generic compression system which will remove entropy just as efficiently.
		// Moreover, these data records are extremely tiny and storing them at full resolution would not take up any 
		// appreciable extra disk space. Some of these engineering decisions are really 'over-engineering' decisions IMO.
		data = readVarint128(data,mVersionNumber);	// Read the version number from a variable length integer source
		data = readVarint128(data,mBlockHeight);	// Read the block height from a variable length integer source
		data = readVarint128(data,mBlockStatus);	// Read the block status bits from a variable length integer source
		data = readVarint128(data,mTransactionCount);	// Read the total number of transactions in this block from a variable length integer source
		// According to documentation provided by Pieter Wuille if bit 8 or bit 16 is set, then we can expect to find a 
		// variable length integer representing which block file this block is located in.
		if ( mBlockStatus & BLOCK_HAVE_MASK)
		{
			data = readVarint128(data,mFileIndex);
		}
		if ( mBlockStatus & BLOCK_HAVE_DATA ) // if bit 8 is set, then we will find an file offset location as a variable length integer next
		{
			data = readVarint128(data,mFileOffset);
		}
		if ( mBlockStatus & BLOCK_HAVE_UNDO ) // if bit 16 is set, we can expect to find a variable length integer corresponding to the file offset location to access 'undo' data for the block
		{
			data = readVarint128(data,mUndoOffset);
		}
		const uint8_t *scan = (const uint8_t *)data;
		mBlockVersion = *(int32_t *)scan; scan+=sizeof(int32_t); // read the version number
		memcpy(mHashPrevious,scan,sizeof(mHashPrevious)); scan+=sizeof(mHashPrevious); // read the hash to the previous block
		memcpy(mHashMerkleRoot,scan,sizeof(mHashMerkleRoot)); scan+=sizeof(mHashMerkleRoot); // read the merkle root hash
		mTime = *(uint32_t *)scan; scan+=sizeof(uint32_t); // read the time stamp for this block
		mBits = *(uint32_t *)scan; scan+=sizeof(uint32_t); // read the bits field
		mNonce = *(uint32_t *)scan; scan+=sizeof(uint32_t); // read the nonce for this block

		size_t size = scan - start;
		assert( size == expectedSize );

		return scan;
	}

	/**
	* reads a variable length integer using a highly customized algorithm unique to the bitcoin core
	* code base! This code matches the method called: ReadVarInt in 'serialize.h' in the bitcoin core source code.
	* 
	* @param data : The pointer to the variable length integer
	* @param value : A reference to a 64 bit integer to return the decompressed value
	* 
	* @return : Returns the address in memory immediately after reading the variable length integer
	*/
	inline const void *readVarint128(const void *data,uint64_t &value)
	{
		const uint8_t *ret = (const uint8_t *)data;	// Cast the void pointer to a single byte pointer

		value = 0;		// Set the return value to zero initially
		uint8_t byte=0; // Each byte we are streaming in
		do
		{
			byte = *ret++;	// Get the byte value and or the 7 lower bits into the result.
			value = (value<<7) | uint64_t(byte&0x7F);
			// If this is not the last byte, we increment the value we are accumulating by one.
			// This is extremely non-standard and I could not find a reference to this technique
			// anywhere online other than the bitcoin source code itself.

			if ( byte & 0x80 )
			{
				value++;
			}
		} while ( byte & 0x80 );


		return ret; // Return the new pointer location
	}

	// For debugging purposes, this method will print out the results of the block which
	// you can compare to the actual blocks you might find on a blockchain explorer
	void printInfo(void) const
	{
		printf("BlockHash       : "); printHash(mBlockHash);
		printf("VersionNumber   : %d\n", uint32_t(mVersionNumber));
		printf("BlockHeight     : %d\n", uint32_t(mBlockHeight));
		printf("BlockStatus     : %08X\n", uint32_t(mBlockStatus));
		printf("TransactionCount: %d\n", uint32_t(mTransactionCount));
		printf("FileIndex       : %d\n", uint32_t(mFileIndex));
		printf("FileOffset      : %d\n", uint32_t(mFileOffset));
		printf("UndoOffset      : %d\n", uint32_t(mUndoOffset));
		printf("BlockVersion    : %d\n", uint32_t(mBlockVersion));
		printf("HashPrevious    : "); printHash(mHashPrevious);
		printf("HashMerkleRoot  : "); printHash(mHashMerkleRoot);
		printf("BlockTime       : "); printTime(mTime);
		printf("Bits            : %08X\n", mBits);
		printf("Nonce           : %08X\n", mNonce);
	}

	void printHash(const uint8_t *hash) const
	{
		for (uint32_t i=0; i<32; i++)
		{
			uint32_t index = 31-i;
			printf("%02X", uint32_t(hash[index]));
		}
		printf("\n");
	}

	void printTime(uint32_t _t) const
	{
		time_t t(_t);
		struct tm *gtm = gmtime(&t);
		printf("%4d-%02d-%02d\n", gtm->tm_year + 1900, gtm->tm_mon + 1, gtm->tm_mday);
	}

	/**
	* Returns the block time as YEAR-MONTH-DAY as and std::string
	*/
	std::string getBlockTime(void) const
	{
		time_t t(mTime);
		struct tm *gtm = gmtime(&t);
		char scratch[512];
		snprintf(scratch,sizeof(scratch),"%4d-%02d-%02d", gtm->tm_year + 1900, gtm->tm_mon + 1, gtm->tm_mday);
		return std::string(scratch);
	}
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif
