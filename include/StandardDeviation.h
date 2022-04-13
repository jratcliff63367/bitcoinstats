#pragma once

#include <stdint.h>
#include <float.h>
#include <math.h>

inline double computeStandardDeviation(uint32_t count,const double *points,double &mean)
{
    double ret = 0;

    double sum = 0;
    for (uint32_t i=0; i<count; i++)
    {
        sum+=points[i];
    }
    mean = sum / double(count);
    for (uint32_t i=0; i<count; i++)
    {
        double diff = points[i] - mean;
        ret+=(diff*diff);
    }
    ret = sqrt(ret) / double(count);

    return ret;
}
