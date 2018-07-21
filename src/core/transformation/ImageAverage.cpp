
// include system headers
// ...

// include own headers
#include "ImageAverage.h"

// include application headers
#include "PlanarImageCalc.h"

// include 3rd party headers
#ifdef USE_LOG4CXX
#include <log4cxx/logger.h>
#endif //USE_LOG4CXX

namespace imageshrink
{

#ifdef USE_LOG4CXX
static log4cxx::LoggerPtr loggerTransformation ( log4cxx::Logger::getLogger( "transformation" ) );
#endif //USE_LOG4CXX

ImageAverage::ImageAverage()
: m_averaging( 8 )
, m_pixelFormat( PixelFormat::UNKNOWN )
, m_colorspace( Colorspace::UNKNOWN  )
, m_bitsPerPixelAndChannel( BitsPerPixelAndChannel::UNKNOWN )
, m_chrominanceSubsampling( ChrominanceSubsampling::UNKNOWN )
, m_imageBuffer()
, m_width( 0 )
, m_height( 0 )
{
    reset();
}

ImageAverage::ImageAverage( ImageInterfaceShrdPtr image, int averaging )
: ImageAverage( *image, averaging )
{
    // nothing
}

ImageAverage::ImageAverage( const ImageInterface & image, int averaging )
: m_averaging( averaging )
, m_pixelFormat( PixelFormat::UNKNOWN )
, m_colorspace( Colorspace::UNKNOWN  )
, m_bitsPerPixelAndChannel( BitsPerPixelAndChannel::UNKNOWN )
, m_chrominanceSubsampling( ChrominanceSubsampling::UNKNOWN )
, m_imageBuffer()
, m_width( 0 )
, m_height( 0 )
{
    reset();
    ImageAverage avg;

    switch( image.getPixelFormat() )
    {
        case PixelFormat::RGB:
            avg = calcAverageImage_RGB( image );
            break;

        case PixelFormat::YCbCr_Planar:
            avg = calcAverageImage_YUV( image );
            break;

        default:
#ifdef USE_LOG4CXX
            LOG4CXX_ERROR( loggerTransformation, "unknown pixelformat " << PixelFormat::toString( image.getPixelFormat() ) );
#endif //USE_LOG4CXX
            break;
    }

    m_pixelFormat            = avg.m_pixelFormat;
    m_colorspace             = avg.m_colorspace;
    m_bitsPerPixelAndChannel = avg.m_bitsPerPixelAndChannel;
    m_chrominanceSubsampling = avg.m_chrominanceSubsampling;
    m_imageBuffer            = avg.m_imageBuffer;
    m_width                  = avg.m_width;
    m_height                 = avg.m_height;
}

void ImageAverage::reset()
{
    m_pixelFormat = PixelFormat::UNKNOWN;
    m_colorspace = Colorspace::UNKNOWN;
    m_bitsPerPixelAndChannel = BitsPerPixelAndChannel::UNKNOWN;
    m_chrominanceSubsampling = ChrominanceSubsampling::UNKNOWN;
    m_imageBuffer.reset();
}

ImageAverage ImageAverage::calcAverageImage_RGB( const ImageInterface & image )
{
    ImageAverage ret;
    
    // check original image
    ImageBufferShrdPtr imageBuffer = image.getImageBuffer();

    if( !imageBuffer )
    {
#ifdef USE_LOG4CXX
        LOG4CXX_ERROR( loggerTransformation, "imageBuffer is a nullptr" );
#endif //USE_LOG4CXX
        ret.reset();
        return ret;
    }

    // check colorspace
    if( image.getColorspace() != Colorspace::RGB )
    {
#ifdef USE_LOG4CXX
        LOG4CXX_ERROR( loggerTransformation, "colorspace is not RGB" );
#endif //USE_LOG4CXX
        ret.reset();
        return ret;
    }

    // check pixel format
    if( image.getPixelFormat() != PixelFormat::RGB )
    {
#ifdef USE_LOG4CXX
        LOG4CXX_ERROR( loggerTransformation, "pixel-format is not RGB" );
#endif //USE_LOG4CXX
        ret.reset();
        return ret;
    }

    // determine buffer size and check it
    const int bytesPerPixel = PixelFormat::channelsPerPixel(image.getPixelFormat()) * BitsPerPixelAndChannel::bytesPerChannel(image.getBitsPerPixelAndChannel());
    const int nofPixels     = image.getWidth() * image.getHeight();
    const int bufferSize    = bytesPerPixel * nofPixels;

    if( bufferSize != imageBuffer->size )
    {
#ifdef USE_LOG4CXX
        LOG4CXX_ERROR( loggerTransformation, "buffer size mismatch (" << __FILE__ << ", " << __LINE__ << ")" );
#endif //USE_LOG4CXX
        ret.reset();
        return ret;
    }

    // determine new image
    const int newWidth  = image.getWidth() / m_averaging;
    const int newHeight = image.getHeight() / m_averaging;

    const int newNofPixels  = newWidth * newHeight;
    const int newBufferSize = bytesPerPixel * newNofPixels;

#ifdef USE_LOG4CXX
    LOG4CXX_INFO( loggerTransformation, "original image buffer size: "
                                        << bufferSize 
                                        << " Bytes"
                                        << "; new image buffer size: "
                                        << newBufferSize
                                        << " Bytes"
    );
#endif //USE_LOG4CXX

    ImageBufferShrdPtr newImageBuffer = std::make_shared<ImageBuffer>( newBufferSize );

#ifdef USE_LOG4CXX
    LOG4CXX_INFO( loggerTransformation, "averaging ..." );
#endif //USE_LOG4CXX

    int bytesPerLine = image.getWidth() * bytesPerPixel;
    int bytesPerNewLine = bytesPerLine / m_averaging;

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

            for( int xWindow = 0; xWindow < m_averaging; ++xWindow )
            {
                const int xWindowByteOffset = bytesPerPixel * ( xNew * m_averaging + xWindow );

                for( int yWindow = 0; yWindow < m_averaging; ++yWindow )
                {
                    const int yWindowByteOffset = bytesPerLine * ( yNew * m_averaging + yWindow );
                    sumAvgCh1 += imageBuffer->image[ xWindowByteOffset + yWindowByteOffset + 0 ];
                    sumAvgCh2 += imageBuffer->image[ xWindowByteOffset + yWindowByteOffset + 1 ];
                    sumAvgCh3 += imageBuffer->image[ xWindowByteOffset + yWindowByteOffset + 2 ];
                }
            }

            const int avgAvg = m_averaging * m_averaging;
            newImageBuffer->image[ xNewByteOffset + yNewByteOffset + 0 ] = sumAvgCh1 / avgAvg;
            newImageBuffer->image[ xNewByteOffset + yNewByteOffset + 1 ] = sumAvgCh2 / avgAvg;
            newImageBuffer->image[ xNewByteOffset + yNewByteOffset + 2 ] = sumAvgCh3 / avgAvg;
        }
    }

#ifdef USE_LOG4CXX
    LOG4CXX_INFO( loggerTransformation, "averaging ... done" );
#endif //USE_LOG4CXX

    // collect data
    ret.m_pixelFormat            = image.getPixelFormat();
    ret.m_colorspace             = image.getColorspace();
    ret.m_bitsPerPixelAndChannel = image.getBitsPerPixelAndChannel();
    ret.m_chrominanceSubsampling = image.getChrominanceSubsampling();
    ret.m_imageBuffer            = newImageBuffer;
    ret.m_width                  = newWidth;
    ret.m_height                 = newHeight;

    return ret;
}

ImageAverage ImageAverage::calcAverageImage_YUV( const ImageInterface & image )
{
    ImageAverage ret;

    // check original image
    ImageBufferShrdPtr imageBuffer = image.getImageBuffer();

    if( !imageBuffer )
    {
#ifdef USE_LOG4CXX
        LOG4CXX_ERROR( loggerTransformation, "imageBuffer is a nullptr" );
#endif //USE_LOG4CXX
        ret.reset();
        return ret;
    }

    // check color space
    if( image.getColorspace() != Colorspace::YCbCr )
    {
#ifdef USE_LOG4CXX
        LOG4CXX_ERROR( loggerTransformation, "colorspace is not YCbCr" );
#endif //USE_LOG4CXX
        ret.reset();
        return ret;
    }

    // check pixel format
    if( image.getPixelFormat() != PixelFormat::YCbCr_Planar )
    {
#ifdef USE_LOG4CXX
        LOG4CXX_ERROR( loggerTransformation, "pixel-format is not YCbCr_Planar" );
#endif //USE_LOG4CXX
        ret.reset();
        return ret;
    }

    // preparation
    const ChrominanceSubsampling::VALUE cs = image.getChrominanceSubsampling();

    int chromaAveragingX = m_averaging;
    int chromaAveragingY = m_averaging;

    switch( cs )
    {
        case ChrominanceSubsampling::CS_444:
            break;

        case ChrominanceSubsampling::CS_422:
            chromaAveragingX = m_averaging / 2;
            chromaAveragingY = m_averaging / 1;
            break;

        case ChrominanceSubsampling::CS_420:
            chromaAveragingX = m_averaging / 2;
            chromaAveragingY = m_averaging / 2;
            break;

        default:
#ifdef USE_LOG4CXX
            LOG4CXX_ERROR( loggerTransformation, "not supported chrominance subsampling " << ChrominanceSubsampling::toString( cs ) );
#endif //USE_LOG4CXX
            ret.reset();
            return ret;
            break;
    }

    const int oldWidth  = image.getWidth();
    const int oldHeight = image.getHeight();
    
    const int newWidth  = oldWidth / m_averaging;
    const int newHeight = oldHeight / m_averaging;

    PlanarImageDesc planaImageNew = calcPlanaerImageDescForYUV( newWidth, newHeight, cs, TJ_PAD );
    PlanarImageDesc planaImageOld = calcPlanaerImageDescForYUV( oldWidth, oldHeight, cs, TJ_PAD );

    ImageBufferShrdPtr imageBufferNew = std::make_shared<ImageBuffer>( planaImageNew.bufferSize );
    ImageBufferShrdPtr imageBufferOld = image.getImageBuffer();

    if(    ( !imageBufferNew )
        || ( !imageBufferOld )
      )
    {
#ifdef USE_LOG4CXX
        LOG4CXX_ERROR( loggerTransformation, "imageBuffer is a nullptr" );
#endif //USE_LOG4CXX
        ret.reset();
        return ret;
    }

    // if( ( planaImageNew.planeSize0 + planaImageNew.planeSize1 + planaImageNew.planeSize2 ) != planaImageNew.bufferSize )
    // {
    //     LOG4CXX_ERROR( loggerTransformation, ""
    //         << planaImageNew.planeSize0 << std::endl
    //         << planaImageNew.planeSize1 << std::endl
    //         << planaImageNew.planeSize2 << std::endl
    //         << planaImageNew.bufferSize << std::endl
    //     );
    //     LOG4CXX_ERROR( loggerTransformation, "buffer size mismatch (" << __FILE__ << ", " << __LINE__ << ")" );
    //     return ret;
    // }


#ifdef USE_LOG4CXX
    LOG4CXX_INFO( loggerTransformation, "original image buffer size: "
                                        << planaImageOld.bufferSize 
                                        << " Bytes"
                                        << "; new image buffer size: "
                                        << planaImageNew.bufferSize
                                        << " Bytes"
    );
#endif //USE_LOG4CXX

#ifdef USE_LOG4CXX
    LOG4CXX_INFO( loggerTransformation, "averaging ..." );
#endif //USE_LOG4CXX

    unsigned char * const plane0New = &imageBufferNew->image[ 0 ];
    unsigned char * const plane1New = &imageBufferNew->image[ planaImageNew.planeSize0 ];
    unsigned char * const plane2New = &imageBufferNew->image[ planaImageNew.planeSize0 + planaImageNew.planeSize1 ];

    const unsigned char * const plane0Old = &imageBufferOld->image[ 0 ];
    const unsigned char * const plane1Old = &imageBufferOld->image[ planaImageOld.planeSize0 ];
    const unsigned char * const plane2Old = &imageBufferOld->image[ planaImageOld.planeSize0 + planaImageOld.planeSize1 ];

    // determine new image
    #pragma omp parallel sections
    {
        // create chroma plane
        #pragma omp section
        {
            const int bytesPerPixel = 1;
            const int bytesPerNewLine = planaImageNew.stride0;
            const int bytesPerOldLine = planaImageOld.stride0;

            #pragma omp parallel for
            for( int yNew = 0; yNew < planaImageNew.height0; ++yNew )
            {
                for( int xNew = 0; xNew < planaImageNew.width0; ++xNew )
                {
                    int sum = 0;

                    for( int yOldOffset = 0; yOldOffset < m_averaging; ++yOldOffset )
                    {
                        const int yOld = yNew * m_averaging + yOldOffset;

                        for( int xOldOffset = 0; xOldOffset < m_averaging; ++xOldOffset )
                        {
                            const int xOld = xNew * m_averaging + xOldOffset;

                            const int xOldByteOffset = bytesPerPixel * xOld;
                            const int yOldByteOffset = bytesPerOldLine * yOld;

                            sum += plane0Old[ xOldByteOffset + yOldByteOffset ];
                        }
                    }

                    const int xNewByteOffset = bytesPerPixel * xNew;
                    const int yNewByteOffset = bytesPerNewLine * yNew;

                    plane0New[ xNewByteOffset + yNewByteOffset ] = sum / ( m_averaging * m_averaging );
                }
            }
        }

        // create U/Cb plane
        #pragma omp section
        {
            const int bytesPerPixel = 1;
            const int bytesPerNewLine = planaImageNew.stride1;
            const int bytesPerOldLine = planaImageOld.stride1;

            #pragma omp parallel for
            for( int yNew = 0; yNew < planaImageNew.height1; ++yNew )
            {
                for( int xNew = 0; xNew < planaImageNew.width1; ++xNew )
                {
                    int sum = 0;

                    for( int yOldOffset = 0; yOldOffset < chromaAveragingY; ++yOldOffset )
                    {
                        const int yOld = yNew * chromaAveragingY + yOldOffset;

                        for( int xOldOffset = 0; xOldOffset < chromaAveragingX; ++xOldOffset )
                        {
                            const int xOld = xNew * chromaAveragingX + xOldOffset;

                            const int xOldByteOffset = bytesPerPixel * xOld;
                            const int yOldByteOffset = bytesPerOldLine * yOld;

                            sum += plane1Old[ xOldByteOffset + yOldByteOffset ];
                        }
                    }

                    const int xNewByteOffset = bytesPerPixel * xNew;
                    const int yNewByteOffset = bytesPerNewLine * yNew;

                    plane1New[ xNewByteOffset + yNewByteOffset ] = sum / ( chromaAveragingX * chromaAveragingY );
                }
            }
        }

        // create V/Cr plane
        #pragma omp section
        {
            const int bytesPerPixel = 1;
            const int bytesPerNewLine = planaImageNew.stride2;
            const int bytesPerOldLine = planaImageOld.stride2;

            #pragma omp parallel for
            for( int yNew = 0; yNew < planaImageNew.height2; ++yNew )
            {
                for( int xNew = 0; xNew < planaImageNew.width2; ++xNew )
                {
                    int sum = 0;

                    for( int yOldOffset = 0; yOldOffset < chromaAveragingY; ++yOldOffset )
                    {
                        const int yOld = yNew * chromaAveragingY + yOldOffset;

                        for( int xOldOffset = 0; xOldOffset < chromaAveragingX; ++xOldOffset )
                        {
                            const int xOld = xNew * chromaAveragingX + xOldOffset;

                            const int xOldByteOffset = bytesPerPixel * xOld;
                            const int yOldByteOffset = bytesPerOldLine * yOld;

                            sum += plane2Old[ xOldByteOffset + yOldByteOffset ];
                        }
                    }

                    const int xNewByteOffset = bytesPerPixel * xNew;
                    const int yNewByteOffset = bytesPerNewLine * yNew;

                    plane2New[ xNewByteOffset + yNewByteOffset ] = sum / ( chromaAveragingX * chromaAveragingY );
                }
            }
        }
    }

#ifdef USE_LOG4CXX
    LOG4CXX_INFO( loggerTransformation, "averaging ... done" );
#endif //USE_LOG4CXX

    // collect information
    ret.m_pixelFormat = image.getPixelFormat();
    ret.m_colorspace = image.getColorspace();
    ret.m_bitsPerPixelAndChannel = image.getBitsPerPixelAndChannel();
    ret.m_chrominanceSubsampling = cs;
    ret.m_imageBuffer = imageBufferNew;
    ret.m_width = newWidth;
    ret.m_height = newHeight;

    return ret;
}

} //namespace imageshrink
