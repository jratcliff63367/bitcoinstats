#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <stdint.h>

namespace stonks
{

class Price
{
public:
	double		mPrice{0};		// The price on this date
	std::string mDate;			// The date as a string
	uint32_t	mDateIndex{0};	// The date-index
};

using PriceHistory = std::unordered_map<uint32_t, Price >;


class Stock
{
public:

	bool getPrice(uint32_t tradingDay,Price &price) const
	{
		bool ret = false;

		PriceHistory::const_iterator found = mHistory.find(tradingDay);
		if ( found != mHistory.end() )
		{
			ret = true;
			price = (*found).second;
		}

		return ret;
	}

	std::string		mSymbol;	// The stock symbol
	std::string		mAssetType;
	std::string		mName;
	std::string		mDescription;
	std::string		mCIK;
	std::string		mExchange;
	std::string		mCurrency;
	std::string		mCountry;
	std::string		mSector;
	std::string		mIndustry;
	std::string		mAddress;
	std::string		mFiscalYearEnd;
	std::string		mLatestQuarter;
	double			mMarketCapitalization{0};
	double			mEBITDA{0};
	double			mPERatio{0};
	double			mPEGRatio{0};
	double			mBookValue{0};
	double			mDividendPerShare{0};
	double			mDividendYield{0};
	double			mEPS{0};
	double			mRevenuePerShareTTM{0};
	double			mProfitMargin{0};
	double			mOperatingMarginTTM{0};
	double			mReturnOnAssetsTTM{0};
	double			mReturnOnEquityTTM{0};
	double			mRevenueTTM{0};
	double			mGrossProfitTTM{0};
	double			mDilutedEPSTTM{0};
	double			mQuarterlyEarningsGrowthYOY{0};
	double			mQuarterlyRevenueGrowthYOY{0};
	double			mAnalystTargetPrice{0};
	double			mTrailingPE{0};
	double			mForwardPE{0};
	double			mPriceToSalesRatioTTM{0};
	double			mPriceToBookRatio{0};
	double			mEVToRevenue{0};
	double			mEVToEBITDA{0};
	double			mBeta{0};
	double			m52WeekHigh{0};
	double			m52WeekLow{0};
	double			m50DayMovingAverage{0};
	double			m200DayMovingAverage{0};
	double			mSharesOutstanding{0};
	std::string		mDividendDate;
	std::string		mExDividendDate;
	std::string		mJSON;		// The JSON data which desribes this stock
	uint32_t		mStartDate{0};
	uint32_t		mEndDate{0};
	PriceHistory	mHistory;	// The price history of this stock
};

class Stonks
{
public:
	static Stonks *create(void);

	virtual const Stock *getStock(const std::string &symbol) const = 0;

	virtual uint32_t dateToIndex(const char *date) const = 0;
	virtual const char *indexToDate(uint32_t index) const = 0;
	virtual uint32_t getCurrentDay(void) const = 0;

	virtual uint32_t begin(void) = 0; // begin iterating stock symbols, returns the number available.
	virtual const Stock *next(void) = 0; // goes to the next
	virtual const Stock *find(const std::string &symbol) const = 0;



	virtual void backup(void) = 0;
	virtual void restore(void) = 0;

	virtual void showSectors(void) = 0;
	virtual void showIndustries(void) = 0;

	virtual void release(void) = 0;
protected:
	virtual ~Stonks(void)
	{
	}
};

}
