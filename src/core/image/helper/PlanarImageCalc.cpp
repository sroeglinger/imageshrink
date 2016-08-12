
// include own headers
#include "enumChrominanceSubsampling.h"

// include own headers
#include "PlanarImageCalc.h"

// include 3rd party headers
#include <turbojpeg.h>
#include <log4cxx/logger.h>

namespace imageshrink
{

static log4cxx::LoggerPtr loggerImage( log4cxx::Logger::getLogger( "image" ) );

PlanarImageDesc calcPlanaerImageDescForYUV( int width, int height, ChrominanceSubsampling::VALUE cs, int padding )
{
    PlanarImageDesc ret;

    const int subsampNew = convert2Tj( cs );

    ret.bufferSize = tjBufSizeYUV2( width, padding, height, subsampNew );

    ret.width0 = tjPlaneWidth( 0 /*0 = Y*/,    width, subsampNew );
    ret.width1 = tjPlaneWidth( 1 /*1 = U/Cb*/, width, subsampNew );
    ret.width2 = tjPlaneWidth( 2 /*2 = V/Cr*/, width, subsampNew );

    ret.height0 = tjPlaneHeight( 0 /*0 = Y*/,    height, subsampNew );
    ret.height1 = tjPlaneHeight( 1 /*1 = U/Cb*/, height, subsampNew );
    ret.height2 = tjPlaneHeight( 2 /*2 = V/Cr*/, height, subsampNew );

    ret.stride0 = linePadding( ret.width0, padding );
    ret.stride1 = linePadding( ret.width1, padding );
    ret.stride2 = linePadding( ret.width2, padding );

    ret.planeSize0 = tjPlaneSizeYUV( 0 /*0 = Y*/,    width, ret.stride0, height, subsampNew );
    ret.planeSize1 = tjPlaneSizeYUV( 1 /*1 = U/Cb*/, width, ret.stride1, height, subsampNew );
    ret.planeSize2 = tjPlaneSizeYUV( 2 /*2 = V/Cr*/, width, ret.stride2, height, subsampNew );

    return ret;
}

int convert2Tj( ChrominanceSubsampling::VALUE cs )
{
    switch( cs )
    {
        case ChrominanceSubsampling::CS_444: return TJSAMP_444;
        case ChrominanceSubsampling::CS_422: return TJSAMP_422;
        case ChrominanceSubsampling::CS_420: return TJSAMP_420;
        case ChrominanceSubsampling::Gray:   return TJSAMP_GRAY;
        case ChrominanceSubsampling::CS_440: return TJSAMP_440;
        case ChrominanceSubsampling::CS_411: return TJSAMP_411;
        default: 
            LOG4CXX_INFO( loggerImage, "There is no TurboJpeg Value for " << ChrominanceSubsampling::toString( cs ) << "." );
            return TJSAMP_444;
    }    
}

int linePadding( int width, int padding )
{
    const int mod = width % padding;
    if( mod == 0 )
    {
        return width;
    }
    else
    {
        return width + ( padding - mod );
    }
}

} //namespace imageshrink

