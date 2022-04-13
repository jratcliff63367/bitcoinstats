#include "TradingSimulator.h"
#include "stonks.h"
#include "sutil.h"
#include "StandardDeviation.h"

#include <map>
#include <unordered_map>
#include <vector>
#include <queue>

#pragma warning(disable:4100)

#define SHOW_TRADES 0

namespace stonks
{

enum SymbolState
{
	none,		// do not own this stock and have never owned it
	own		// currently own some of this stock
};

class PendingCash
{
public:
	double	mAmount{0};
	uint32_t mTradingDay{0};
};

using PendingCashQueue = std::queue< PendingCash >;

class StockSymbol
{
public:
	std::string		mSymbol;
	stonks::Price	mStartingPrice;
	stonks::Price	mEndingPrice;
	double			mSharesOwned{0};	// number of shares owned
	double			mSharesPrice{0};	// price paid for them
	SymbolState	mState{SymbolState::none};
	double		mLastPrice{0}; // last price we bought or sold at..
	uint32_t	mLastDate{0};	// last trading day
	double		mCurrentPrice{0};
};

using StockSymbolMap = std::unordered_map< std::string, StockSymbol >;

class Account
{
public:

	Account(const TradingParameters &params,
			const StockList &stocks,
			Stonks *st)
	{
		mParams = params;
		mStonks = st;
		mSettledCash = mParams.mStartingCash;
		for (auto &i:stocks)
		{
			const stonks::Stock *s = st->find(i);
			if ( s )
			{
				StockSymbol ss;
				if( s->getPrice(params.mStartTradingDay,ss.mStartingPrice) && s->getPrice(params.mEndTradingDay,ss.mEndingPrice))
				{
					ss.mSymbol = s->mSymbol;
					ss.mLastDate = params.mStartTradingDay;
					ss.mLastPrice = ss.mStartingPrice.mPrice;
					mStocks[ss.mSymbol] = ss;
				}
			}
		}
		mCurrentDay = mParams.mStartTradingDay;
		double fraction = 1.0 / double(mStocks.size());
		double total = 0;
		double capital = mParams.mStartingCash*mParams.mInitialCapitalAllocation;
		for (auto &i:mStocks)
		{
			double budget = params.mStartingCash * fraction;
			double shares = budget / i.second.mStartingPrice.mPrice;
			double currentValue = shares * i.second.mEndingPrice.mPrice;
			total+=currentValue;

			double c = capital * fraction;
			double scount = c / i.second.mStartingPrice.mPrice;
			i.second.mState = SymbolState::own;
			i.second.mSharesOwned = scount;
			i.second.mSharesPrice = c;
			i.second.mLastDate = mCurrentDay;
			mSettledCash-=c;
		}
		mHoldResults = total;
		//printf("Portfolio Return: $%s\n", sutil::formatNumber(int32_t(total)) );
	}

	bool simulateDay(void)
	{
		bool ret = false;

		if ( mCurrentDay > mParams.mEndTradingDay )
		{
			mTradeResults = mUnsettledCash + mSettledCash;
			for (auto &i:mStocks)
			{
				double position = i.second.mSharesOwned*i.second.mCurrentPrice;
				mTradeResults+=position;
			}

//			printf("Simulation complete: Portfolio Value: $%s vs. Holding:$%s\n", 
//				sutil::formatNumber(int32_t(mTradeResults)),
//				sutil::formatNumber(int32_t(mHoldResults)));

			double percentTrade = ((mTradeResults-mParams.mStartingCash)*100) / mParams.mStartingCash;
			double percentHold = ((mHoldResults-mParams.mStartingCash)*100) / mParams.mStartingCash;

			mImprovement = percentTrade - percentHold;

//			const char *day = mStonks->indexToDate(mParams.mStartTradingDay);
//			printf("%s : [%0.2f%%] HOLD:%0.2f%% TRADE:%0.2f%%\n",day, mImprovement, percentHold, percentTrade);

			ret = true;
		}
		else
		{
			class PriceChange
			{
			public:
				bool operator<(const PriceChange &a) const
				{
					return a.mPercent < mPercent;
				}
				double	mPercent{0};
				std::string	mSymbol;
			};

			// Sort by maximum percentage price drop
			std::priority_queue< PriceChange > pchange;
			for (auto &i:mStocks)
			{
				StockSymbol &ss = i.second;
				const stonks::Stock *s = mStonks->find(ss.mSymbol);
				if ( s )
				{
					Price p;
					if( s->getPrice(mCurrentDay,p))
					{
						ss.mCurrentPrice = p.mPrice; // what the current price is
						double diff = p.mPrice - ss.mLastPrice; // what is the difference to the reference price...
						double percentDifference = (diff*100) / ss.mLastPrice; // percentage change
						PriceChange pc;
						pc.mPercent = percentDifference;
						pc.mSymbol = ss.mSymbol;
						pchange.push(pc);
					}
				}
			}

			while ( !pchange.empty() )
			{
				PriceChange pcg = pchange.top();
				pchange.pop();
				StockSymbolMap::iterator found = mStocks.find(pcg.mSymbol);
				if ( found != mStocks.end() )
				{
					StockSymbol &ss = (*found).second;
					const stonks::Stock *s = mStonks->find(ss.mSymbol);
					if ( s )
					{
						Price p;
						if( s->getPrice(mCurrentDay,p))
						{
							ss.mCurrentPrice = p.mPrice; // what the current price is
							double diff = p.mPrice - ss.mLastPrice; // what is the difference to the reference price...
							double percentDifference = (diff*100) / ss.mLastPrice; // percentage change
							// If we do not currently own this stock, either update the last price of it's higher than what we are currently tracking, or
							// decide this is a buying opportunity
							if ( ss.mState == SymbolState::none ) 
							{
								if ( percentDifference <= mParams.mPercentFirstBuy )
								{
									int32_t shares =(int32_t) (mParams.mFirstBuy / p.mPrice)+1;  //how many shares to buy...
									double cost = double(shares)*p.mPrice;
									if ( cost <= mSettledCash )
									{
										ss.mState = SymbolState::own;
										ss.mSharesOwned+=double(shares);
										ss.mSharesPrice+=cost;
										ss.mLastPrice = p.mPrice;
										ss.mLastDate = mCurrentDay;
										mSettledCash-=cost;
#if SHOW_TRADES
										printf("Bought %d shares of %s for $%s\n", shares, ss.mSymbol.c_str(), sutil::formatNumber(int32_t(cost)));
#endif
									}
								}
								if ( p.mPrice > ss.mLastPrice )
								{
									ss.mLastPrice = p.mPrice;
								}
							}
							else if ( ss.mState == SymbolState::own )
							{
								double currentValue = ss.mSharesOwned*p.mPrice;
								double diffValue = currentValue - ss.mSharesPrice;
								double percentIncrease = (diffValue*100) / ss.mSharesPrice;
								uint32_t daysSinceFirstBought = mCurrentDay - ss.mLastDate;

								if ( percentIncrease <= mParams.mMaxLossPercentage && daysSinceFirstBought >= mParams.mMaxLossDays && 0  )
								{
									mUnsettledCash+=currentValue;
									PendingCash pc;
									pc.mTradingDay = mCurrentDay;
									pc.mAmount = currentValue;
									mPendingCash.push(pc);

									double profit = currentValue - ss.mSharesPrice;
									mProfitTaken+=profit;
#if SHOW_TRADES || 1
									printf("Sold %d shares of %s for $%s taking the loss of:$%s : TotalProfit:$%s\n", 
										int32_t(ss.mSharesOwned), 
										ss.mSymbol.c_str(), 
										sutil::formatNumber(currentValue),
										sutil::formatNumber(-profit),
										sutil::formatNumber(mProfitTaken));
#endif
									ss.mState = SymbolState::none;
									ss.mLastPrice = p.mPrice;
									ss.mSharesOwned = 0;
									ss.mSharesPrice = 0;
								}
								else if ( percentDifference <= mParams.mPercentRebuy )
								{
									// We cannot rebuy if that would cause us to go over our maximum buy limit
									if ( ss.mSharesPrice < (mParams.mMaxBuy-mParams.mRebuy) )
									{
										int32_t shares =(int32_t) (mParams.mRebuy / p.mPrice)+1;  //how many shares to buy...
										double cost = double(shares)*p.mPrice;
										if ( cost <= mSettledCash )
										{
											ss.mState = SymbolState::own;
											ss.mLastPrice = p.mPrice;
											ss.mSharesOwned+=double(shares);
											ss.mSharesPrice+=cost;
											mSettledCash-=cost;
#if SHOW_TRADES
											printf("Bought more %d shares of %s for $%s\n", shares, ss.mSymbol.c_str(), sutil::formatNumber(int32_t(cost)));
#endif
										}
									}
								}
								else
								{
									if ( percentIncrease >= mParams.mTakeProfit)
									{
										mUnsettledCash+=currentValue;
										PendingCash pc;
										pc.mTradingDay = mCurrentDay;
										pc.mAmount = currentValue;
										mPendingCash.push(pc);
										double profit = currentValue - ss.mSharesPrice;
										mProfitTaken+=profit;
#if SHOW_TRADES
										printf("Sold %d shares of %s for $%s banking profit of:$%s : TotalProfit:$%s\n", 
											int32_t(ss.mSharesOwned), 
											ss.mSymbol.c_str(), 
											sutil::formatNumber(currentValue),
											sutil::formatNumber(profit),
											sutil::formatNumber(mProfitTaken));
#endif
										ss.mState = SymbolState::none;
										ss.mLastPrice = p.mPrice;
										ss.mSharesOwned = 0;
										ss.mSharesPrice = 0;

									}
								}
							}
						}
					}
				}
			}
			processPendingCash();
			mCurrentDay++;
		}

		return ret;
	}

	void processPendingCash(void)
	{
		while ( !mPendingCash.empty() )
		{
			PendingCash pc = mPendingCash.front();
			uint32_t diff = mCurrentDay - pc.mTradingDay;
			if ( diff >= 3 )
			{
				mSettledCash+=pc.mAmount;
				mUnsettledCash-=pc.mAmount;
#if 0
				printf("Settled:$%s : Unsettled:$%s\n",
					sutil::formatNumber(int32_t(mSettledCash)),
					sutil::formatNumber(int32_t(mUnsettledCash)));
#endif
				mPendingCash.pop();
			}
			else
			{
				break;
			}
		}
	}

	TradingParameters	mParams;
	uint32_t		mCurrentDay{0};
	double			mHoldResults{0}; // how much money we would have if we just bought and held over this period of time
	double			mTradeResults{0};
	double			mImprovement{0};
	double			mSettledCash{0};
	double			mUnsettledCash{0};
	StockSymbolMap	mStocks;
	PendingCashQueue mPendingCash;
	Stonks			*mStonks{nullptr};
	double			mProfitTaken{0};
};

class TradingSimulatorImpl : public TradingSimulator
{
public:

	virtual double runSimulation(const TradingParameters &params,
								const StockList &stocks,
								Stonks *s) final
	{
		double ret = 0;

		TradingParameters p = params;
		uint32_t stopDay = p.mEndTradingDay - 120;
		(stopDay);

		uint32_t simCount=0;
		uint32_t winCount=0;

		std::vector< double > results;

		for (uint32_t i=params.mStartTradingDay; i<stopDay; i++)
		{
			p.mStartTradingDay = i;
			p.mEndTradingDay = i+120;

			Account a(p,stocks,s);

			while ( !a.simulateDay() )
			{
				// run simulation for each trading day
			}

			if ( a.mTradeResults > a.mHoldResults )
			{
				winCount++;
			}

			results.push_back(a.mImprovement);
			simCount++;
		}

		printf("Out of %d simulations %d days were winners.\n", simCount, winCount);

		double mean;
		double stdev = computeStandardDeviation(uint32_t(results.size()),&results[0],mean);
		printf("Standard Deviation:%0.2f%% Mean:%0.2f%%\n", stdev, mean );

		ret = mean;

		return ret;
	}

	virtual void release(void) final
	{
		delete this;
	}
};

TradingSimulator *TradingSimulator::create(void)
{
	auto ret = new TradingSimulatorImpl;
	return static_cast< TradingSimulator *>(ret);
}


}
