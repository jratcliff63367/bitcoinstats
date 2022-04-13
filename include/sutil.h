#pragma once

#include <stdint.h>

// string utils
namespace sutil
{

const char *   formatNumber(int32_t number); // JWR  format this integer into a fancy comma delimited string
const char *   formatNumber(uint32_t number); // JWR  format this integer into a fancy comma delimited string
const char * formatNumber(double number); // JWR  format this integer into a fancy comma delimited string

}
