
// include system headers

// include own headers
#include "ImageDummy.h"

// include 3rd party headers
#ifdef USE_LOG4CXX
#include <log4cxx/logger.h>
#endif //USE_LOG4CXX

namespace imageshrink
{

#ifdef USE_LOG4CXX
static log4cxx::LoggerPtr loggerImage( log4cxx::Logger::getLogger( "image" ) );
#endif //USE_LOG4CXX
    
ImageDummy::ImageDummy()
: m_pixelFormat( PixelFormat::UNKNOWN )
, m_colorspace( Colorspace::UNKNOWN  )
, m_bitsPerPixelAndChannel( BitsPerPixelAndChannel::UNKNOWN )
, m_chrominanceSubsampling( ChrominanceSubsampling::UNKNOWN )
, m_imageBuffer()
, m_width( 0 )
, m_height( 0 )
{
    reset();
}

ImageDummy::ImageDummy( const ImageInterface & image )
: m_pixelFormat( PixelFormat::UNKNOWN )
, m_colorspace( Colorspace::UNKNOWN  )
, m_bitsPerPixelAndChannel( BitsPerPixelAndChannel::UNKNOWN )
, m_chrominanceSubsampling( ChrominanceSubsampling::UNKNOWN )
, m_imageBuffer()
, m_width( 0 )
, m_height( 0 )
{
    m_pixelFormat            = image.getPixelFormat();
    m_colorspace             = image.getColorspace();
    m_bitsPerPixelAndChannel = image.getBitsPerPixelAndChannel();
    m_chrominanceSubsampling = image.getChrominanceSubsampling();
    m_imageBuffer            = image.getImageBuffer();
    m_width                  = image.getWidth();
    m_height                 = image.getHeight();
}

void ImageDummy::reset()
{
    m_pixelFormat = PixelFormat::UNKNOWN;
    m_colorspace = Colorspace::UNKNOWN;
    m_bitsPerPixelAndChannel = BitsPerPixelAndChannel::UNKNOWN;
    m_chrominanceSubsampling = ChrominanceSubsampling::UNKNOWN;
    m_imageBuffer.reset();
}

} //namespace imageshrink
