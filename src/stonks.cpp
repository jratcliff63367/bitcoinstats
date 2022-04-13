#include "stonks.h"
#include "KeyValueDatabase.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "StandardDeviation.h"

#include <map>
#include <set>
#include <vector>
#include <assert.h>

namespace stonks
{


using StocksMap = std::map< std::string, Stock >;
using DateToIndex = std::map< std::string, uint32_t >;
using IndexToDate = std::map< uint32_t, std::string>;
using SectorMap = std::map< std::string, uint32_t >;
using IndustryMap = std::map< std::string, uint32_t >;

class StonksImpl : public Stonks
{
public:
	StonksImpl(void)
	{
		//auto database = keyvaluedatabase::KeyValueDatabase::create("d:\\github\\stonks\\stock-market");
		auto database = keyvaluedatabase::KeyValueDatabase::create("d:\\github\\stonks\\stonks");
		assert(database);
		if ( database )
		{
			printf("Scanning for all valid market dates using AAPL as a baseline refrerence.\n");
			bool ok = database->begin("price.AAPL.");
			assert(ok);
			while ( ok )
			{
				std::string key;
				std::string value;
				ok = database->next(key,value);
				if ( ok )
				{
					const char *date = getDate(key.c_str());
					assert(date);
					if ( date )
					{
						uint32_t index = uint32_t(mDateToIndex.size())+1;
						std::string sdate(date);
						mDateToIndex[sdate] = index;
						mIndexToDate[index] = sdate;
					}
				}
			}
			printf("Processed %d unique market dates.\n", uint32_t(mDateToIndex.size()));
		}
		if ( database )
		{
			printf("Reading price data for all tickers.\n");
			bool ok = database->begin("ticker.");
			assert(ok);
			std::vector< std::string > tickers;
			while ( ok )
			{
				std::string key;
				std::string value;
				ok = database->next(key,value);
				if ( ok )
				{
					const char *ticker = strchr(key.c_str(),'.');
					assert(ticker);
					if ( ticker )
					{
						ticker++;
						if ( strcmp(value.c_str(),"no_price") == 0 )
						{
							//printf("Skipping ticker(%s) with no valid price data.\n", ticker );
						}
						else
						{
							Stock s;
							s.mSymbol = std::string(ticker);
							s.mJSON = value;
							parseStock(s);
							mStocks[s.mSymbol] = s;
							tickers.push_back(std::string(ticker));
						}
					}
				}
			}
			for (auto &i:tickers)
			{
				readPriceHistory(i.c_str(),database);
			}
			database->release();
		}
	}

	virtual ~StonksImpl(void)
	{
	}

	virtual const Stock *getStock(const std::string &symbol) const final
	{
		const Stock *ret = nullptr;

		StocksMap::const_iterator found = mStocks.find(symbol);
		if ( found != mStocks.end() )
		{
			ret = &(*found).second;
		}
		return ret;
	}

	virtual void release(void) final
	{
		delete this;
	}

	virtual uint32_t begin(void) final // begin iterating stock symbols, returns the number available.
	{
		mIterator = mStocks.begin();
		return uint32_t(mStocks.size());
	}

	virtual const Stock *next(void) final // goes to the next
	{
		const Stock *ret = nullptr;
		if ( mIterator != mStocks.end() )
		{
			ret = &(*mIterator).second;
			mIterator++;
		}
		return ret;
	}

	const char *getDate(const char *scan) const
	{
		const char *ret = nullptr;

		scan = strchr(scan,'.');
		assert(scan);
		if ( scan )
		{
			scan = strchr(scan+1,'.');
			assert(scan);
			if ( scan )
			{
				ret = scan+1;
			}
		}


		return ret;
	}

	virtual uint32_t dateToIndex(const char *date) const final
	{
		uint32_t ret = 0;

		if ( date )
		{
			DateToIndex::const_iterator found = mDateToIndex.find(std::string(date));
			if ( found != mDateToIndex.end() )
			{
				ret = (*found).second;
			}
		}

		return ret;
	}

	virtual const char *indexToDate(uint32_t index) const final
	{
		const char *ret = nullptr;

		IndexToDate::const_iterator found = mIndexToDate.find(index);
		if ( found != mIndexToDate.end() )
		{
			ret = (*found).second.c_str();
		}

		return ret;
	}

	uint32_t readPriceHistory(const char *ticker,keyvaluedatabase::KeyValueDatabase *database)
	{
		uint32_t ret = 0;

		std::string skipKey = "price." + std::string(ticker);
		std::string prefix = skipKey + std::string(".");
		bool ok = database->begin(prefix.c_str());
		assert(ok);
		if ( ok )
		{
			PriceHistory phistory;
			uint32_t startDate=0;
			uint32_t endDate = 0;

			while ( ok )
			{
				std::string key;
				std::string value;
				ok = database->next(key,value);
				if ( ok )
				{
					Price p;
					p.mPrice = atof(value.c_str());
					if ( key == skipKey )
					{
					}
					else
					{
						const char *date = getDate(key.c_str());
						if ( date )
						{
							p.mDate = std::string(date);
							p.mDateIndex = dateToIndex(date);
							if ( p.mDateIndex )
							{
								assert(p.mDateIndex);

								if ( startDate == 0 )
								{
									startDate = p.mDateIndex;
									endDate = p.mDateIndex;
								}
								else
								{
									if ( p.mDateIndex < startDate )
									{
										startDate = p.mDateIndex;
									}
									if ( p.mDateIndex > endDate )
									{
										endDate = p.mDateIndex;
									}
								}

								phistory[p.mDateIndex] = p;
								ret++;
							}
							else
							{
								//printf("Unable to resolve date:%s\n", date );
							}
						}
					}
				}
			}
			std::string sticker(ticker);
			StocksMap::iterator found =  mStocks.find(sticker);
			if ( found != mStocks.end() )
			{
				(*found).second.mHistory = phistory;
				(*found).second.mStartDate = startDate;
				(*found).second.mEndDate = endDate;
			}
			else
			{
				assert(0);
			}
		}
		//printf("Found %d price dates for ticker %s.\n", ret, ticker);

		return ret;
	}

	virtual void backup(void) final
	{
		FILE *fph = fopen("d:\\github\\stonks\\backup\\tickers.csv","wb");
		if ( fph )
		{
			printf("Saving stock tickers.\n");
			for (auto &i:mStocks)
			{
				auto &stock = i.first;
				fprintf(fph,"%s\n", stock.c_str());
			}
			fclose(fph);
		}
		else
		{
			assert(0);
		}

		for (auto &i:mStocks)
		{
			auto &stock = i.second;
			char scratch[512];
			snprintf(scratch,sizeof(scratch),"d:\\github\\stonks\\backup\\ticker.%s.json", i.first.c_str());
			FILE *json = fopen(scratch,"wb");
			if ( json )
			{
				printf("Saving stock data:%s\n", scratch);
				fwrite(stock.mJSON.c_str(),stock.mJSON.size(),1,json);
				fclose(json);
			}
			else
			{
				assert(0);
			}
			snprintf(scratch,sizeof(scratch),"d:\\github\\stonks\\backup\\price.%s.csv", i.first.c_str());
			FILE *price = fopen(scratch,"wb");
			if ( price )
			{
				printf("Saving Price History:%s\n", scratch);
				for (auto &j:stock.mHistory)
				{
					fprintf(price,"%s,%0.2f\n", j.second.mDate.c_str(), j.second.mPrice);
				}
				fclose(price);
			}
			else
			{
				assert(0);
			}
		}
		printf("Backup complete.\n");
	}

	virtual void restore(void) final
	{
	}

#define GET_STRING(n,m) if ( document.HasMember(n) ) m = document[n].GetString(); else assert(0);
#define GET_DOUBLE(n,m) if ( document.HasMember(n) ) { const char *str = document[n].GetString(); m = atof(str); } else assert(0);

	bool parseStock(Stock &s)
	{
		bool ret = false;

		rapidjson::Document document;
		document.Parse(s.mJSON.c_str());

		GET_STRING("Symbol",s.mSymbol);
   		GET_STRING("AssetType",s.mAssetType);
   		GET_STRING("Name",s.mName);
   		GET_STRING("Description",s.mDescription);
   		GET_STRING("CIK",s.mCIK);
   		GET_STRING("Exchange",s.mExchange);
   		GET_STRING("Currency",s.mCurrency);
   		GET_STRING("Country",s.mCountry);
   		GET_STRING("Sector",s.mSector);
   		GET_STRING("Industry",s.mIndustry);
   		GET_STRING("Address",s.mAddress);
   		GET_STRING("FiscalYearEnd",s.mFiscalYearEnd);
   		GET_STRING("LatestQuarter",s.mLatestQuarter);
   		GET_DOUBLE("MarketCapitalization",s.mMarketCapitalization);
   		GET_DOUBLE("EBITDA",s.mEBITDA);
   		GET_DOUBLE("PERatio",s.mPERatio);
   		GET_DOUBLE("PEGRatio",s.mPEGRatio);
   		GET_DOUBLE("BookValue",s.mBookValue);
   		GET_DOUBLE("DividendPerShare",s.mDividendPerShare);
   		GET_DOUBLE("DividendYield",s.mDividendYield);
   		GET_DOUBLE("EPS",s.mEPS);
   		GET_DOUBLE("RevenuePerShareTTM",s.mRevenuePerShareTTM);
   		GET_DOUBLE("ProfitMargin",s.mProfitMargin);
   		GET_DOUBLE("OperatingMarginTTM",s.mOperatingMarginTTM);
   		GET_DOUBLE("ReturnOnAssetsTTM",s.mReturnOnAssetsTTM);
   		GET_DOUBLE("ReturnOnEquityTTM",s.mReturnOnEquityTTM);
   		GET_DOUBLE("RevenueTTM",s.mRevenueTTM);
   		GET_DOUBLE("GrossProfitTTM",s.mGrossProfitTTM);
   		GET_DOUBLE("DilutedEPSTTM",s.mDilutedEPSTTM);
   		GET_DOUBLE("QuarterlyEarningsGrowthYOY",s.mQuarterlyEarningsGrowthYOY);
   		GET_DOUBLE("QuarterlyRevenueGrowthYOY",s.mQuarterlyRevenueGrowthYOY);
   		GET_DOUBLE("AnalystTargetPrice",s.mAnalystTargetPrice);
   		GET_DOUBLE("TrailingPE",s.mTrailingPE);
   		GET_DOUBLE("ForwardPE",s.mForwardPE);
   		GET_DOUBLE("PriceToSalesRatioTTM",s.mPriceToSalesRatioTTM);
   		GET_DOUBLE("PriceToBookRatio",s.mPriceToBookRatio);
   		GET_DOUBLE("EVToRevenue",s.mEVToRevenue);
   		GET_DOUBLE("EVToEBITDA",s.mEVToEBITDA);
   		GET_DOUBLE("Beta",s.mBeta);
   		GET_DOUBLE("52WeekHigh",s.m52WeekHigh);
   		GET_DOUBLE("52WeekLow",s.m52WeekLow);
   		GET_DOUBLE("50DayMovingAverage",s.m50DayMovingAverage);
   		GET_DOUBLE("200DayMovingAverage",s.m200DayMovingAverage);
   		GET_DOUBLE("SharesOutstanding",s.mSharesOutstanding);
   		GET_STRING("DividendDate",s.mDividendDate);
   		GET_STRING("ExDividendDate",s.mExDividendDate);

		addSector(s.mSector);
		addIndustry(s.mIndustry);

		return ret;
	}

	void addSector(const std::string &sector)
	{
		SectorMap::iterator found = mSectors.find(sector);
		if ( found == mSectors.end() )
		{
			mSectors[sector] = 1;
		}
		else
		{
			(*found).second++;
		}
	}

	void addIndustry(const std::string &industry)
	{
		IndustryMap::iterator found = mIndustries.find(industry);
		if ( found == mIndustries.end() )
		{
			mIndustries[industry] = 1;
		}
		else
		{
			(*found).second++;
		}
	}


	virtual void showSectors(void) final
	{
		for (auto &i:mSectors)
		{
			printf("%-60s : %10d\n", i.first.c_str(), i.second);
		}
		printf("Unique Sectors:%d\n", uint32_t(mSectors.size()));
	}

	virtual void showIndustries(void) final
	{
		for (auto &i:mIndustries)
		{
			printf("%-60s : %10d\n", i.first.c_str(), i.second);
		}
		printf("Unique Industries:%d\n", uint32_t(mIndustries.size()));
	}

	virtual const Stock *find(const std::string &symbol) const final
	{
		const stonks::Stock *ret = nullptr;

		StocksMap::const_iterator found = mStocks.find(symbol);
		if ( found != mStocks.end() )
		{
			ret = &(*found).second;
		}

		return ret;
	}

	virtual uint32_t getCurrentDay(void) const final
	{
		return uint32_t(mDateToIndex.size());
	}


	StocksMap::iterator mIterator;
	StocksMap	mStocks;
	DateToIndex	mDateToIndex;
	IndexToDate mIndexToDate;
	SectorMap	mSectors;
	IndustryMap	mIndustries;
};

Stonks *Stonks::create(void)
{
	auto ret = new StonksImpl;
	return static_cast< Stonks *>(ret);
}


}
