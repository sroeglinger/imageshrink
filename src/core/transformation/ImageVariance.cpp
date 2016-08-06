
// include system headers
// ...

// include own headers
#include "ImageVariance.h"

// include 3rd party headers
#include <log4cxx/logger.h>

namespace imageshrink
{

static log4cxx::LoggerPtr loggerTransformation ( log4cxx::Logger::getLogger( "transformation" ) );

ImageVariance::ImageVariance()
: m_pixelFormat( PixelFormat::UNKNOWN )
, m_colorspace( Colorspace::UNKNOWN  )
, m_bitsPerPixelAndChannel( BitsPerPixelAndChannel::UNKNOWN )
, m_imageBuffer()
, m_width( 0 )
, m_height( 0 )
{
    reset();
}

ImageVariance::ImageVariance( const ImageInterface & image, const ImageInterface & averageImage )
: m_pixelFormat( PixelFormat::UNKNOWN )
, m_colorspace( Colorspace::UNKNOWN  )
, m_bitsPerPixelAndChannel( BitsPerPixelAndChannel::UNKNOWN )
, m_imageBuffer()
, m_width( 0 )
, m_height( 0 )
{
    reset();
    ImageVariance var = calcVarianceImage( image, averageImage );

    m_pixelFormat            = var.m_pixelFormat;
    m_colorspace             = var.m_colorspace;
    m_bitsPerPixelAndChannel = var.m_bitsPerPixelAndChannel;
    m_imageBuffer            = var.m_imageBuffer;
    m_width                  = var.m_width;
    m_height                 = var.m_height;
}

void ImageVariance::reset()
{
    m_pixelFormat = PixelFormat::UNKNOWN;
    m_colorspace = Colorspace::UNKNOWN;
    m_bitsPerPixelAndChannel = BitsPerPixelAndChannel::UNKNOWN;
    m_imageBuffer.reset();
}

ImageVariance ImageVariance::calcVarianceImage( const ImageInterface & image, const ImageInterface & averageImage )
{
    ImageVariance ret;
    const int averaging = 8;

    // check original image
    ImageBufferShrdPtr imageBuffer    = image.getImageBuffer();
    ImageBufferShrdPtr imageAvgBuffer = averageImage.getImageBuffer();

    if(    ( !imageBuffer )
        || ( !imageAvgBuffer )
      )
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

    if(    ( averageImage.getWidth() != newWidth )
        || ( averageImage.getHeight() != newHeight )
      )
    {
        LOG4CXX_ERROR( loggerTransformation, "size missmatch between original image and average image" );
        ret.reset();
        return ret;
    }

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

    for( int xNew = 0; xNew < newWidth; ++xNew )
    {
        for( int yNew = 0; yNew < newHeight; ++yNew )
        {
            int sumCh1 = 0;
            int sumCh2 = 0;
            int sumCh3 = 0;

            const int xNewByteOffset = bytesPerPixel * xNew;
            const int yNewByteOffset = bytesPerNewLine * yNew;

            for( int xWindow = 0; xWindow < averaging; ++xWindow )
            {
                const int xWindowByteOffset = bytesPerPixel * ( xNew * averaging + xWindow );

                for( int yWindow = 0; yWindow < averaging; ++yWindow )
                {
                    const int yWindowByteOffset = bytesPerLine * ( yNew * averaging + yWindow );

                    int ch1  = imageBuffer->image[ xWindowByteOffset + yWindowByteOffset + 0 ];
                    ch1     -= imageAvgBuffer->image[ xNewByteOffset + yNewByteOffset + 0 ];
                    sumCh1  += ch1 * ch1;

                    int ch2  = imageBuffer->image[ xWindowByteOffset + yWindowByteOffset + 1 ];
                    ch2     -= imageAvgBuffer->image[ xNewByteOffset + yNewByteOffset + 1 ];
                    sumCh2  += ch2 * ch2;

                    int ch3  = imageBuffer->image[ xWindowByteOffset + yWindowByteOffset + 2 ];
                    ch3     -= imageAvgBuffer->image[ xNewByteOffset + yNewByteOffset + 2 ];
                    sumCh3  += ch3 * ch3;
                }
            }

            const int avgAvg = averaging * averaging;
            newImageBuffer->image[ xNewByteOffset + yNewByteOffset + 0 ] = sumCh1 / avgAvg;
            newImageBuffer->image[ xNewByteOffset + yNewByteOffset + 1 ] = sumCh2 / avgAvg;
            newImageBuffer->image[ xNewByteOffset + yNewByteOffset + 2 ] = sumCh3 / avgAvg;
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
