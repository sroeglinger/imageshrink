
// include system headers
// ...

// include own headers
#include "ImageCovariance.h"

// include 3rd party headers
#include <log4cxx/logger.h>

namespace imageshrink
{

static log4cxx::LoggerPtr loggerTransformation ( log4cxx::Logger::getLogger( "transformation" ) );

ImageCovariance::ImageCovariance()
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

ImageCovariance::ImageCovariance( ImageInterfaceShrdPtr image1, ImageInterfaceShrdPtr averageImage1, ImageInterfaceShrdPtr image2, ImageInterfaceShrdPtr averageImage2 )
: ImageCovariance( *image1, *averageImage1, *image2, *averageImage2 )
{
    // nothing
}

ImageCovariance::ImageCovariance( const ImageInterface & image1, const ImageInterface & averageImage1, const ImageInterface & image2, const ImageInterface & averageImage2 )
: m_pixelFormat( PixelFormat::UNKNOWN )
, m_colorspace( Colorspace::UNKNOWN  )
, m_bitsPerPixelAndChannel( BitsPerPixelAndChannel::UNKNOWN )
, m_chrominanceSubsampling( ChrominanceSubsampling::UNKNOWN )
, m_imageBuffer()
, m_width( 0 )
, m_height( 0 )
{
    reset();
    ImageCovariance var = calcCovarianceImage( image1, averageImage1, image2, averageImage2 );

    m_pixelFormat            = var.m_pixelFormat;
    m_colorspace             = var.m_colorspace;
    m_bitsPerPixelAndChannel = var.m_bitsPerPixelAndChannel;
    m_chrominanceSubsampling = var.m_chrominanceSubsampling;
    m_imageBuffer            = var.m_imageBuffer;
    m_width                  = var.m_width;
    m_height                 = var.m_height;
}

void ImageCovariance::reset()
{
    m_pixelFormat = PixelFormat::UNKNOWN;
    m_colorspace = Colorspace::UNKNOWN;
    m_bitsPerPixelAndChannel = BitsPerPixelAndChannel::UNKNOWN;
    m_chrominanceSubsampling = ChrominanceSubsampling::UNKNOWN;
    m_imageBuffer.reset();
}

ImageCovariance ImageCovariance::calcCovarianceImage( const ImageInterface & image1, const ImageInterface & averageImage1, const ImageInterface & image2, const ImageInterface & averageImage2 )
{
    ImageCovariance ret;
    const int averaging = 8;

    // collect buffers
    ImageBufferShrdPtr imageBuffer1    = image1.getImageBuffer();
    ImageBufferShrdPtr imageAvgBuffer1 = averageImage1.getImageBuffer();
    ImageBufferShrdPtr imageBuffer2    = image2.getImageBuffer();
    ImageBufferShrdPtr imageAvgBuffer2 = averageImage2.getImageBuffer();

    // check original image
    if(    ( !imageBuffer1 )
        || ( !imageAvgBuffer1 )
        || ( !imageBuffer2 )
        || ( !imageAvgBuffer2 )
      )
    {
        LOG4CXX_ERROR( loggerTransformation, "imageBuffer is a nullptr" );
        ret.reset();
        return ret;
    }

    if(    ( image1.getWidth() != image2.getWidth() )
        || ( image1.getHeight() != image2.getHeight() )
        || ( averageImage1.getWidth() != averageImage2.getWidth() )
        || ( averageImage1.getHeight() != averageImage2.getHeight() )
        || ( averageImage1.getWidth() != image1.getWidth() / averaging )
        || ( averageImage1.getHeight() != image1.getHeight() / averaging )
      )
    {
        LOG4CXX_ERROR( loggerTransformation, "size missmatch between images" );
        ret.reset();
        return ret;
    }

    const int bytesPerPixel = PixelFormat::channelsPerPixel(image1.getPixelFormat()) * BitsPerPixelAndChannel::bytesPerChannel(image1.getBitsPerPixelAndChannel());
    const int nofPixels     = image1.getWidth() * image1.getHeight();
    const int bufferSize    = bytesPerPixel * nofPixels;

    if(    ( bufferSize != imageBuffer1->size )
        || ( bufferSize != imageBuffer2->size )
      )
    {
        LOG4CXX_ERROR( loggerTransformation, "buffer size missmatch" );
        ret.reset();
        return ret;
    }

    // determine new image
    const int newWidth  = image1.getWidth() / averaging;
    const int newHeight = image1.getHeight() / averaging;

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

    int bytesPerLine = image1.getWidth() * bytesPerPixel;
    int bytesPerNewLine = bytesPerLine / averaging;

    #pragma omp parallel for
    for( int yNew = 0; yNew < newHeight; ++yNew )
    {
        for( int xNew = 0; xNew < newWidth; ++xNew )
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

                    int ch1_1  = imageBuffer1->image[ xWindowByteOffset + yWindowByteOffset + 0 ];
                    ch1_1     -= imageAvgBuffer1->image[ xNewByteOffset + yNewByteOffset + 0 ];
                    int ch1_2  = imageBuffer2->image[ xWindowByteOffset + yWindowByteOffset + 0 ];
                    ch1_2     -= imageAvgBuffer2->image[ xNewByteOffset + yNewByteOffset + 0 ];
                    sumCh1    += ch1_1 * ch1_2;

                    int ch2_1  = imageBuffer1->image[ xWindowByteOffset + yWindowByteOffset + 1 ];
                    ch2_1     -= imageAvgBuffer1->image[ xNewByteOffset + yNewByteOffset + 1 ];
                    int ch2_2  = imageBuffer2->image[ xWindowByteOffset + yWindowByteOffset + 1 ];
                    ch2_2     -= imageAvgBuffer2->image[ xNewByteOffset + yNewByteOffset + 1 ];
                    sumCh2    += ch2_1 * ch2_2;

                    int ch3_1  = imageBuffer1->image[ xWindowByteOffset + yWindowByteOffset + 2 ];
                    ch3_1     -= imageAvgBuffer1->image[ xNewByteOffset + yNewByteOffset + 2 ];
                    int ch3_2  = imageBuffer2->image[ xWindowByteOffset + yWindowByteOffset + 2 ];
                    ch3_2     -= imageAvgBuffer2->image[ xNewByteOffset + yNewByteOffset + 2 ];
                    sumCh3    += ch3_1 * ch3_2;
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
    ret.m_pixelFormat            = image1.getPixelFormat();
    ret.m_colorspace             = image1.getColorspace();
    ret.m_bitsPerPixelAndChannel = image1.getBitsPerPixelAndChannel();
    ret.m_chrominanceSubsampling = image1.getChrominanceSubsampling();
    ret.m_imageBuffer            = newImageBuffer;
    ret.m_width                  = newWidth;
    ret.m_height                 = newHeight;

    return ret;
}

} //namespace imageshrink
