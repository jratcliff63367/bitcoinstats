#include "sutil.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace sutil
{

#define MAXNUMERIC 32  // JWR  support up to 16 32 character long numeric formated strings
#define MAXFNUM    16

static	char  gFormat[MAXNUMERIC*MAXFNUM];
static int32_t    gIndex=0;

const char * formatNumber(int32_t number) // JWR  format this integer into a fancy comma delimited string
{
	char * dest = &gFormat[gIndex*MAXNUMERIC];
	gIndex++;
	if ( gIndex == MAXFNUM ) gIndex = 0;

	char scratch[512];

#if defined (LINUX_GENERIC) || defined(LINUX) || defined(__CELLOS_LV2__)
	snprintf(scratch, 10, "%d", number);
#else
	_itoa(number,scratch,10);
#endif

	char *str = dest;
	uint32_t len = (uint32_t)strlen(scratch);
	for (uint32_t i=0; i<len; i++)
	{
		int32_t place = (len-1)-i;
		*str++ = scratch[i];
		if ( place && (place%3) == 0 ) *str++ = ',';
	}
	*str = 0;

	return dest;
}

const char * formatNumber(double number) // JWR  format this integer into a fancy comma delimited string
{
	return formatNumber(int32_t(number));
}

const char * formatNumber(uint32_t number) // JWR  format this integer into a fancy comma delimited string
{
	return formatNumber(int32_t(number));
}

}
