#pragma once

// simulates trading using the algorithm I have developed
#include <stdint.h>
#include <vector>
#include <string>

namespace stonks
{

using StockList = std::vector< std::string>;

class TradingParameters
{
public:
	double		mInitialCapitalAllocation{0.01};
	uint32_t	mStartTradingDay{0};
	uint32_t	mEndTradingDay{0};
	double		mStartingCash{500000};
	double		mMaxBuy{7700}; // don't spend more than 5k on any one stock
	double		mFirstBuy{7700}; // on the first buy, only 2k
	double		mRebuy{3000};		// on additoinal buys, 1k
	double		mPercentFirstBuy{-2.63}; // initial buy, must be a 5% drop
	double		mPercentRebuy{-3};			// rebuy on an additional 3% drop
	double		mTakeProfit{6.49};		// take profit on 3% gains
	uint32_t	mMaxLossDays{20};	// if a stock doesn't rebound in a month, sell it
	double		mMaxLossPercentage{10}; // if a stock is down by more than 10% after a month, sell it
};

class Stonks;

class TradingSimulator
{
public:
	static TradingSimulator *create(void);

	virtual double runSimulation(const TradingParameters &params,
								 const StockList &stocks,
								 Stonks *s) = 0;

	virtual void release(void) =0;
protected:
	virtual ~TradingSimulator(void)
	{
	}
};

}
