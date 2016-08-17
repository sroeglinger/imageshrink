
#ifndef PLANAERIMAGECALC_H_
#define PLANAERIMAGECALC_H_

// include application headers
#include "enumChrominanceSubsampling.h"

namespace imageshrink
{

static const int TJ_PAD = 4;

struct PlanarImageDesc
{
    int bufferSize;

    int width0;
    int width1;
    int width2;

    int height0;
    int height1;
    int height2;

    int stride0;
    int stride1;
    int stride2;

    int planeSize0;
    int planeSize1;
    int planeSize2;
}; //struct

PlanarImageDesc calcPlanaerImageDescForYUV( int width, int height, ChrominanceSubsampling::VALUE cs, int padding );
int convert2Tj( ChrominanceSubsampling::VALUE cs );
int linePadding( int width, int padding );

} //namespace imageshrink

#endif //PLANAERIMAGECALC_H_
