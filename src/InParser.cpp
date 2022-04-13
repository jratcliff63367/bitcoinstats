#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if CARB_COMPILER_MSC
#    pragma warning(disable : 4996)
#endif

#include "InParser.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/** @file inparser.cpp
 * @brief        Parse ASCII text, in place, very quickly.
 *
 * This class provides for high speed in-place (destructive) parsing of an ASCII text file.
 * This class will either load an ASCII text file from disk, or can be constructed with a pointer to
 * a piece of ASCII text in memory.  It can only be called once, and the contents are destroyed.
 * To speed the process of parsing, it simply builds pointers to the original ascii data and replaces the
 * separators with a zero byte to indicate end of string.  It performs callbacks to parse each line, in argc/argv
 * format, offering the option to cancel the parsing process at any time.
 *
 *
 * By default the only valid separator is whitespace.  It will not treat commas or any other symbol as a separator.
 * However, you can specify up to 32 'hard' separators, such as a comma, equal sign, etc. and these will act as valid
 * separators and come back as part of the argc/argv data list.
 *
 * To use the parser simply inherit the pure virtual base class 'InPlaceParserInterface'.  Define the method
 * 'ParseLine'. When you invoke the Parse method on the InPlaceParser class, you will get an ARGC - ARGV style callback
 * for each line in the source file.  If you return 'false' at any time, it will abort parsing.  The entire thing is
 * stack based, so you can recursively call multiple parser instances.
 *
 * It is important to note.  Since this parser is 'in place' it writes 'zero bytes' (EOS marker) on top of the
 * whitespace. While it can handle text in quotes, it does not handle escape sequences.  This is a limitation which
 * could be resolved. There is a hard coded maximum limit of 512 arguments per line.
 *
 * Here is the full example usage:
 *
 *  InPlaceParser ipp("parse_me.txt");
 *
 *    ipp.Parse(this);
 *
 *  That's it, and you will receive an ARGC - ARGV callback for every line in the file.
 *
 *  If you want to parse some text in memory of your own. (It *MUST* be terminated by a zero byte, and lines seperated
 * by carriage return or line-feed.  You will receive an assertion if it does not.  If you specify the source data than
 * *you* are responsible for that memory and must de-allocate it yourself.  If the data was loaded from a file on disk,
 * then it is automatically de-allocated by the InPlaceParser.
 *
 *  You can also construct the InPlaceParser without passing any data, so you can simply pass it a line of data at a
 * time yourself.  The line of data should be zero-byte terminated.
 */

//==================================================================================

#define DEFAULT_MAX_ARGS 256
#define DEFAULT_STRING_BUFFER_SIZE (1024 * 4)

namespace inparser
{

void InPlaceParser::SetFile(const char* fname)
{
    if (mMyAlloc)
    {
        ::free(mData);
    }
    mData = 0;
    mLen = 0;
    mMyAlloc = false;

    FILE* fph = fopen(fname, "rb");
    if (fph)
    {
        fseek(fph, 0L, SEEK_END);
        mLen = ftell(fph);
        fseek(fph, 0L, SEEK_SET);

        if (mLen)
        {
            mData = (char*)malloc(sizeof(char) * (mLen + 1));
            uint64_t read = (uint64_t)fread(mData, (size_t)mLen, 1, fph);
            if (!read)
            {
                ::free(mData);
                mData = 0;
            }
            else
            {
                mData[mLen] = 0; // zero byte terminate end of file marker.
                mMyAlloc = true;
            }
        }
        fclose(fph);
    }
}

//==================================================================================
InPlaceParser::~InPlaceParser(void)
{
    ::free(mStringBuffer);
    ::free(mArgv);
    if (mMyAlloc)
    {
        ::free(mData);
    }
}

//==================================================================================
bool InPlaceParser::IsHard(char c)
{
    return mHard[(unsigned char)c] == ST_HARD;
}

//==================================================================================
char* InPlaceParser::AddHard(uint64_t& argc, char* foo)
{
    while (IsHard(*foo))
    {
        const char* hard = &mHardString[*foo * 2];
        growArgs(argc);
        mArgv[argc++] = hard;
        ++foo;
    }
    return foo;
}

//==================================================================================
bool InPlaceParser::IsWhiteSpace(char c)
{
    return mHard[(unsigned char)c] == ST_SOFT;
}

//==================================================================================
char* InPlaceParser::SkipSpaces(char* foo)
{
    while (!EOS(*foo) && IsWhiteSpace(*foo))
        ++foo;
    return foo;
}

//==================================================================================
bool InPlaceParser::IsNonSeparator(char c)
{
    return (!IsHard(c) && !IsWhiteSpace(c) && c != 0);
}

//==================================================================================
uint64_t InPlaceParser::ProcessLine(uint64_t lineno, char* line, InPlaceParserInterface* callback)
{
    uint64_t ret = 0;

    uint64_t argc = 0;

    char* foo = line;
    while (!EOS(*foo))
    {
        foo = SkipSpaces(foo); // skip any leading spaces
        if (EOS(*foo))
            break;

        if (*foo == mQuoteChar) // if it is an open quote
        {
            ++foo;
            growArgs(argc);
            mArgv[argc++] = foo;
            while (!EOS(*foo) && *foo != mQuoteChar)
                ++foo;
            if (!EOS(*foo))
            {
                *foo = 0; // replace close quote with zero byte EOS
                ++foo;
            }
        }
        else
        {
            foo = AddHard(argc, foo); // add any hard separators, skip any spaces

            if (IsNonSeparator(*foo)) // add non-hard argument.
            {
                bool quote = false;
                if (*foo == mQuoteChar)
                {
                    ++foo;
                    quote = true;
                }
                growArgs(argc);
                mArgv[argc++] = foo;

                if (quote)
                {
                    while (*foo && *foo != mQuoteChar)
                        ++foo;
                    if (*foo)
                        *foo = 32;
                }

                // continue..until we hit an eos ..
                while (!EOS(*foo)) // until we hit EOS
                {
                    if (IsWhiteSpace(*foo)) // if we hit a space, stomp a zero byte, and exit
                    {
                        *foo = 0;
                        ++foo;
                        break;
                    }
                    else if (IsHard(*foo)) // if we hit a hard separator, stomp a zero byte and store the hard separator
                                           // argument
                    {
                        const char* hard = &mHardString[*foo * 2];
                        *foo = 0;
                        growArgs(argc);
                        mArgv[argc++] = hard;
                        ++foo;
                        break;
                    }
                    ++foo;
                } // end of while loop...
            }
        }
    }
    if (argc)
    {
        ret = callback->ParseLine(lineno, argc, mArgv);
    }

    return ret;
}


uint64_t InPlaceParser::Parse(const char* str, InPlaceParserInterface* callback) // returns true if entire file was
                                                                                 // parsed, false if it aborted for some
                                                                                 // reason
{
    uint64_t ret = 0;

    uint64_t dataLen = (uint64_t)strlen(str); // Find out the length of the input string
    if (dataLen)
    {
        setParseData(str, dataLen);
        ret = Parse(callback);
    }

    return ret;
}

//==================================================================================
// returns true if entire file was parsed, false if it aborted for some reason
//==================================================================================
uint64_t InPlaceParser::Parse(InPlaceParserInterface* callback)
{
    uint64_t ret = 0;
    assert(callback);
    if (mData)
    {
        uint64_t lineno = 0;

        char* foo = mData;
        char* begin = foo;
        while (*foo)
        {
            if (isLineFeed(*foo))
            {
                ++lineno;
                *foo = 0;
                if (*begin) // if there is any data to parse at all...
                {
                    bool snarfed = callback->preParseLine(lineno, begin);
                    if (!snarfed)
                    {
                        uint64_t v = ProcessLine(lineno, begin, callback);
                        if (v)
                        {
                            ret = v;
                        }
                    }
                }

                ++foo;
                if (*foo == '\n')
                    ++foo; // skip line feed, if it is in the carriage-return line-feed format...
                begin = foo;
            }
            else
            {
                ++foo;
            }
        }

        lineno++; // last line.
        bool snarfed = callback->preParseLine(lineno, begin);
        if (!snarfed)
        {
            uint64_t v = ProcessLine(lineno, begin, callback);
            if (v)
                ret = v;
        }
    }
    return ret;
}

//==================================================================================
void InPlaceParser::DefaultSymbols(bool useComment)
{
    SetHard(',');
    SetHard('(');
    SetHard(')');
    SetHard('=');
    SetHard('[');
    SetHard(']');
    SetHard('{');
    SetHard('}');
    if (useComment)
    {
        SetCommentSymbol('#');
    }
}

//==================================================================================
// convert source string into an arg list, this is a destructive parse.
//==================================================================================
const char** InPlaceParser::GetArglist(char* line, uint64_t& count)
{
    const char** ret = 0;

    uint64_t argc = 0;

    char* foo = line;

    while (!EOS(*foo))
    {
        foo = SkipSpaces(foo); // skip any leading spaces

        if (EOS(*foo))
            break;

        if (*foo == mQuoteChar) // if it is an open quote
        {
            ++foo;
            growArgs(argc);
            mArgv[argc++] = foo;
            while (!EOS(*foo) && *foo != mQuoteChar)
                ++foo;
            if (!EOS(*foo))
            {
                *foo = 0; // replace close quote with zero byte EOS
                ++foo;
            }
        }
        else
        {
            foo = AddHard(argc, foo); // add any hard separators, skip any spaces

            if (IsNonSeparator(*foo)) // add non-hard argument.
            {
                bool quote = false;
                if (*foo == mQuoteChar)
                {
                    ++foo;
                    quote = true;
                }
                growArgs(argc);
                mArgv[argc++] = foo;

                if (quote)
                {
                    while (*foo && *foo != mQuoteChar)
                        ++foo;
                    if (*foo)
                        *foo = 32;
                }

                // continue..until we hit an eos ..
                while (!EOS(*foo)) // until we hit EOS
                {
                    if (IsWhiteSpace(*foo)) // if we hit a space, stomp a zero byte, and exit
                    {
                        *foo = 0;
                        ++foo;
                        break;
                    }
                    else if (IsHard(*foo)) // if we hit a hard separator, stomp a zero byte and store the hard separator
                                           // argument
                    {
                        const char* hard = &mHardString[*foo * 2];
                        *foo = 0;
                        growArgs(argc);
                        mArgv[argc++] = hard;
                        ++foo;
                        break;
                    }
                    ++foo;
                } // end of while loop...
            }
        }
    }

    count = argc;
    if (argc)
    {
        ret = mArgv;
    }

    return ret;
}

// Data is *const*.  We make a copy and add a zero byte terminator at the end
void InPlaceParser::setParseData(const void* data, uint64_t dataLen)
{
    if ((dataLen + 1) > mStringLen) // If the string is larger than our current buffer
    {
        mStringLen = (dataLen + 1) * 2; // Allocate a buffer twice as large as needed
        ::free(mStringBuffer);
        mStringBuffer = (char*)malloc(mStringLen);
    }
    // If we allocated the current data buffer, we need to free it.
    if (mMyAlloc)
    {
        ::free(mData);
    }
    mData = mStringBuffer;
    mLen = dataLen;
    memcpy(mData, data, mLen);
    mData[mLen] = 0;
}

void InPlaceParser::growArgs(uint64_t argc)
{
    if (argc >= mMaxArgs)
    {
        mMaxArgs = mMaxArgs * 2;
        const char** argv = (const char**)malloc(sizeof(const char*) * mMaxArgs);
        for (uint64_t i = 0; i < argc; i++)
        {
            argv[i] = mArgv[i];
        }
        ::free(mArgv);
        mArgv = argv;
    }
}

void InPlaceParser::Init(void)
{
    mQuoteChar = 34;
    mData = 0;
    mLen = 0;
    mMyAlloc = false;
    for (uint64_t i = 0; i < 256; i++)
    {
        mHard[i] = ST_DATA;
        mHardString[i * 2] = (char)i;
        mHardString[i * 2 + 1] = 0;
    }
    mHard[0] = ST_EOS;
    mHard[32] = ST_SOFT;
    mHard[9] = ST_SOFT;
    mHard[13] = ST_LINE_FEED;
    mHard[10] = ST_LINE_FEED;

    mMaxArgs = DEFAULT_MAX_ARGS;
    mArgv = (const char**)malloc(sizeof(const char*) * mMaxArgs);

    mStringLen = DEFAULT_STRING_BUFFER_SIZE;
    mStringBuffer = (char*)malloc(mStringLen);
}

} // end of namespace
