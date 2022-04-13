#ifndef RAND_H

#define RAND_H

//** Random number class and Random number pool class.  This acts as a
//** replacement for the stdlib rand() function.  Why would you want to
//** replace the rand() function?  Because you often want a deteriminstic
//** random number generator that is exactly the same regardless of what
//** machine or compiler you build your code on.  The stdlib rand() function
//** is not guaraenteed to produce the same results on different stdlib
//** implementations.
//**
//** You can also maintain any number of unique random number generators
//** as state machines by instancing this rand class over and over again.
//**
//** The random number pool is a data structure which you allocate if you
//** want to pull items from a data set, randomly, but without any
//** duplications.  A simple example would be a deck of cards.  You would
//** instantiate the random number pool as follows:
//**
//** RandPool deck(52);
//**
//** You would then pull cards from the deck as follows:
//**
//** bool shuffled;
//** int card = deck.Get(shuffled);
//**
//** This will return a number between 0-51 (representing a card in the deck)
//** without ever reporting the same card twice until the deck has been
//** exhausted.  If the boolean 'shuffled' is true, then the deck was
//** re-shuffled on that call.  This data structure has lots of uses in
//** computer games where you want to randomly select data from a fixed
//** pool size.
//**
//** This code submitted to FlipCode.com on July 23, 2000 by John W. Ratcliff
//** It is released into the public domain on the same date.

class Rand
{
public:

	Rand(int seed=0)
  {
    mCurrent = seed;
  };

	int Get(void)
  {
    return( (mCurrent = mCurrent * 214013L + 2531011L)  & 0x7fffffff);
  };

  // random number between 0.0 and 1.0
  float Ranf(void)
  {
  	int v = Get()&0x7FFF;
    return (float)v*(1.0f/32767.0f);
  };

  void Set(int seed)
  {
    mCurrent = seed;
  };

private:
	int mCurrent;
};

class RandPool
{
public:
  RandPool(int size,int seed)  // size of random number bool.
  {
    mRand.Set(seed);       // init random number generator.
    mData = new int[size]; // allocate memory for random number bool.
    mSize = size;
    mTop  = mSize;
    for (int i=0; i<mSize; i++) mData[i] = i;
  }
  ~RandPool(void)
  {
    delete [] mData;
  };

  // pull a number from the random number pool, will never return the
  // same number twice until the 'deck' (pool) has been exhausted.
  // Will set the shuffled flag to true if the deck/pool was exhausted
  // on this call.
  int Get(bool &shuffled)
  {
    if ( mTop == 0 ) // deck exhausted, shuffle deck.
    {
      shuffled = true;
      mTop = mSize;
    }
    else
      shuffled = false;
    int entry = mRand.Get()%mTop;
    mTop--;
    int ret      = mData[entry]; // swap top of pool with entry
    mData[entry] = mData[mTop];  // returned
    mData[mTop]  = ret;
    return ret;
  };

	float Ranf(void) { return mRand.Ranf(); };

private:
  Rand mRand;  // random number generator.
  int  *mData;  // random number bool.
  int   mSize;  // size of random number pool.
  int   mTop;   // current top of the random number pool.
};

#endif

