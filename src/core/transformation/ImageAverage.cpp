
// include system headers
// ...

// include own headers
#include "ImageAverage.h"

// include 3rd party headers
#include <log4cxx/logger.h>

namespace imageshrink
{

static log4cxx::LoggerPtr loggerTransformation ( log4cxx::Logger::getLogger( "transformation" ) );

ImageAverage::ImageAverage()
: m_pixelFormat( PixelFormat::UNKNOWN )
, m_colorspace( Colorspace::UNKNOWN  )
, m_bitsPerPixelAndChannel( BitsPerPixelAndChannel::UNKNOWN )
, m_imageBuffer()
, m_width( 0 )
, m_height( 0 )
{
    reset();
}

ImageAverage::ImageAverage( ImageInterfaceShrdPtr image )
: ImageAverage( *image )
{
    // nothing
}

ImageAverage::ImageAverage( const ImageInterface & image )
: m_pixelFormat( PixelFormat::UNKNOWN )
, m_colorspace( Colorspace::UNKNOWN  )
, m_bitsPerPixelAndChannel( BitsPerPixelAndChannel::UNKNOWN )
, m_imageBuffer()
, m_width( 0 )
, m_height( 0 )
{
    reset();
    ImageAverage avg = calcAverageImage( image );

    m_pixelFormat            = avg.m_pixelFormat;
    m_colorspace             = avg.m_colorspace;
    m_bitsPerPixelAndChannel = avg.m_bitsPerPixelAndChannel;
    m_imageBuffer            = avg.m_imageBuffer;
    m_width                  = avg.m_width;
    m_height                 = avg.m_height;
}

void ImageAverage::reset()
{
    m_pixelFormat = PixelFormat::UNKNOWN;
    m_colorspace = Colorspace::UNKNOWN;
    m_bitsPerPixelAndChannel = BitsPerPixelAndChannel::UNKNOWN;
    m_imageBuffer.reset();
}

ImageAverage ImageAverage::calcAverageImage( const ImageInterface & image )
{
    ImageAverage ret;
    const int averaging = 8;

    // check original image
    ImageBufferShrdPtr imageBuffer = image.getImageBuffer();

    if( !imageBuffer )
    {
        LOG4CXX_ERROR( loggerTransformation, "imageBuffer is a nullptr" );
        ret.reset();
        return ret;
    }

    const int bytesPerPixel = PixelFormat::channelsPerPixel(image.getPixelFormat()) * BitsPerPixelAndChannel::bytesPerChannel(image.getBitsPerPixelAndChannel());
    const int nofPixels     = image.getWidth() * image.getHeight();
    const int bufferSize    = bytesPerPixel * nofPixels;

    if( bufferSize != imageBuffer->size )
    {
        LOG4CXX_ERROR( loggerTransformation, "buffer size missmatch" );
        ret.reset();
        return ret;
    }

    // determine new image
    const int newWidth  = image.getWidth() / averaging;
    const int newHeight = image.getHeight() / averaging;

    const int newNofPixels  = newWidth * newHeight;
    const int newBufferSize = bytesPerPixel * newNofPixels;

    LOG4CXX_INFO( loggerTransformation, "original image buffer size: "
                                        << bufferSize 
                                        << " Bytes"
                                        << "; new image buffer size: "
                                        << newBufferSize
                                        << " Bytes"
    );

    ImageBufferShrdPtr newImageBuffer = ImageBufferShrdPtr( new ImageBuffer( newBufferSize ) );

    LOG4CXX_INFO( loggerTransformation, "averaging ..." );

    int bytesPerLine = image.getWidth() * bytesPerPixel;
    int bytesPerNewLine = bytesPerLine / averaging;

    #pragma omp parallel for
    for( int yNew = 0; yNew < newHeight; ++yNew )
    {
        for( int xNew = 0; xNew < newWidth; ++xNew )
        {
            int sumAvgCh1 = 0;
            int sumAvgCh2 = 0;
            int sumAvgCh3 = 0;

            const int xNewByteOffset = bytesPerPixel * xNew;
            const int yNewByteOffset = bytesPerNewLine * yNew;

            for( int xWindow = 0; xWindow < averaging; ++xWindow )
            {
                const int xWindowByteOffset = bytesPerPixel * ( xNew * averaging + xWindow );

                for( int yWindow = 0; yWindow < averaging; ++yWindow )
                {
                    const int yWindowByteOffset = bytesPerLine * ( yNew * averaging + yWindow );
                    sumAvgCh1 += imageBuffer->image[ xWindowByteOffset + yWindowByteOffset + 0 ];
                    sumAvgCh2 += imageBuffer->image[ xWindowByteOffset + yWindowByteOffset + 1 ];
                    sumAvgCh3 += imageBuffer->image[ xWindowByteOffset + yWindowByteOffset + 2 ];
                }
            }

            const int avgAvg = averaging * averaging;
            newImageBuffer->image[ xNewByteOffset + yNewByteOffset + 0 ] = sumAvgCh1 / avgAvg;
            newImageBuffer->image[ xNewByteOffset + yNewByteOffset + 1 ] = sumAvgCh2 / avgAvg;
            newImageBuffer->image[ xNewByteOffset + yNewByteOffset + 2 ] = sumAvgCh3 / avgAvg;
        }
    }

    LOG4CXX_INFO( loggerTransformation, "averaging ... done" );

    // collect data
    ret.m_pixelFormat            = image.getPixelFormat();
    ret.m_colorspace             = image.getColorspace();
    ret.m_bitsPerPixelAndChannel = image.getBitsPerPixelAndChannel();
    ret.m_imageBuffer            = newImageBuffer;
    ret.m_width                  = newWidth;
    ret.m_height                 = newHeight;

    return ret;
}

} //namespace imageshrink
