
// include system headers
// ...

// include own headers
#include "ImageVariance.h"

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

ImageVariance::ImageVariance()
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

ImageVariance::ImageVariance( const ImageInterface & image, const ImageInterface & averageImage )
: m_pixelFormat( PixelFormat::UNKNOWN )
, m_colorspace( Colorspace::UNKNOWN  )
, m_bitsPerPixelAndChannel( BitsPerPixelAndChannel::UNKNOWN )
, m_chrominanceSubsampling( ChrominanceSubsampling::UNKNOWN )
, m_imageBuffer()
, m_width( 0 )
, m_height( 0 )
{
    reset();
    ImageVariance var;

    switch( image.getPixelFormat() )
    {
        case PixelFormat::RGB:
            var = calcVarianceImage_RGB( image, averageImage );
            break;

        case PixelFormat::YCbCr_Planar:
            var = calcVarianceImage_YUV( image, averageImage );
            break;

        default:
#ifdef USE_LOG4CXX
            LOG4CXX_ERROR( loggerTransformation, "unknown pixelformat " << PixelFormat::toString( image.getPixelFormat() ) );
#endif //USE_LOG4CXX
            break;
    }

    m_pixelFormat            = var.m_pixelFormat;
    m_colorspace             = var.m_colorspace;
    m_bitsPerPixelAndChannel = var.m_bitsPerPixelAndChannel;
    m_chrominanceSubsampling = var.m_chrominanceSubsampling;
    m_imageBuffer            = var.m_imageBuffer;
    m_width                  = var.m_width;
    m_height                 = var.m_height;
}

void ImageVariance::reset()
{
    m_pixelFormat = PixelFormat::UNKNOWN;
    m_colorspace = Colorspace::UNKNOWN;
    m_bitsPerPixelAndChannel = BitsPerPixelAndChannel::UNKNOWN;
    m_chrominanceSubsampling = ChrominanceSubsampling::UNKNOWN;
    m_imageBuffer.reset();
}

ImageVariance ImageVariance::calcVarianceImage_RGB( const ImageInterface & image, const ImageInterface & averageImage )
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
#ifdef USE_LOG4CXX
        LOG4CXX_ERROR( loggerTransformation, "imageBuffer is a nullptr" );
#endif //USE_LOG4CXX
        ret.reset();
        return ret;
    }

    // check colorspace
    if(    ( image.getColorspace() != Colorspace::RGB )
        || ( averageImage.getColorspace() != Colorspace::RGB )
      )
    {
#ifdef USE_LOG4CXX
        LOG4CXX_ERROR( loggerTransformation, "colorspace is not RGB" );
#endif //USE_LOG4CXX
        ret.reset();
        return ret;
    }

    // check pixel format
    if(    ( image.getPixelFormat() != PixelFormat::RGB )
        || ( averageImage.getPixelFormat() != PixelFormat::RGB )
      )
    {
#ifdef USE_LOG4CXX
        LOG4CXX_ERROR( loggerTransformation, "pixel-format is not RGB" );
#endif //USE_LOG4CXX
        ret.reset();
        return ret;
    }

    if(    ( averageImage.getWidth() != image.getWidth() / averaging )
        || ( averageImage.getHeight() != image.getHeight() / averaging )
      )
    {
#ifdef USE_LOG4CXX
        LOG4CXX_ERROR( loggerTransformation, "size missmatch between original image and average image" );
#endif //USE_LOG4CXX
        ret.reset();
        return ret;
    }

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
    const int newWidth  = image.getWidth() / averaging;
    const int newHeight = image.getHeight() / averaging;

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
    LOG4CXX_INFO( loggerTransformation, "determine variance ..." );
#endif //USE_LOG4CXX

    int bytesPerLine = image.getWidth() * bytesPerPixel;
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

#ifdef USE_LOG4CXX
    LOG4CXX_INFO( loggerTransformation, "determine variance ... done" );
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

ImageVariance ImageVariance::calcVarianceImage_YUV( const ImageInterface & image, const ImageInterface & averageImage )
{
    ImageVariance ret;
    const int averaging = 8;

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
    if(    ( image.getColorspace() != Colorspace::YCbCr )
        || ( averageImage.getColorspace() != Colorspace::YCbCr )
      )
    {
#ifdef USE_LOG4CXX
        LOG4CXX_ERROR( loggerTransformation, "colorspace is not YCbCr" );
#endif //USE_LOG4CXX
        ret.reset();
        return ret;
    }

    // check pixel format
    if(    ( image.getPixelFormat() != PixelFormat::YCbCr_Planar )
        || ( averageImage.getPixelFormat() != PixelFormat::YCbCr_Planar )
      )
    {
#ifdef USE_LOG4CXX
        LOG4CXX_ERROR( loggerTransformation, "pixel-format is not YCbCr_Planar" );
#endif //USE_LOG4CXX
        ret.reset();
        return ret;
    }

    // preparation
    const ChrominanceSubsampling::VALUE cs = image.getChrominanceSubsampling();

    int chromaAveragingX = averaging;
    int chromaAveragingY = averaging;

    switch( cs )
    {
        case ChrominanceSubsampling::CS_444:
            break;

        case ChrominanceSubsampling::CS_422:
            chromaAveragingX = averaging / 2;
            chromaAveragingY = averaging / 1;
            break;

        case ChrominanceSubsampling::CS_420:
            chromaAveragingX = averaging / 2;
            chromaAveragingY = averaging / 2;
            break;

        default:
#ifdef USE_LOG4CXX
            LOG4CXX_ERROR( loggerTransformation, "not supported chrominance subsampling " << ChrominanceSubsampling::toString( cs ) );
#endif //USE_LOG4CXX
            ret.reset();
            return ret;
            break;
    }

    // check chrominance subsampling
    if(    ( image.getChrominanceSubsampling() != cs )
        && ( averageImage.getChrominanceSubsampling() != cs )
      )
    {
#ifdef USE_LOG4CXX
        LOG4CXX_ERROR( loggerTransformation, "chrominance subsampling mismatch" );
#endif //USE_LOG4CXX
        ret.reset();
        return ret;
    }

    // preparation

    const int oldWidth  = image.getWidth();
    const int oldHeight = image.getHeight();
    
    const int newWidth  = oldWidth / averaging;
    const int newHeight = oldHeight / averaging;

    PlanarImageDesc planaImageNew = calcPlanaerImageDescForYUV( newWidth, newHeight, cs, TJ_PAD );
    PlanarImageDesc planaImageOld = calcPlanaerImageDescForYUV( oldWidth, oldHeight, cs, TJ_PAD );

    ImageBufferShrdPtr imageBufferNew = std::make_shared<ImageBuffer>( planaImageNew.bufferSize );
    ImageBufferShrdPtr imageBufferOld = image.getImageBuffer();
    ImageBufferShrdPtr imageAvgBuffer = averageImage.getImageBuffer();

    if(    ( !imageBufferNew )
        || ( !imageBufferOld )
        || ( !imageAvgBuffer )
      )
    {
#ifdef USE_LOG4CXX
        LOG4CXX_ERROR( loggerTransformation, "imageBuffer is a nullptr" );
#endif //USE_LOG4CXX
        ret.reset();
        return ret;
    }

    if(    ( averageImage.getWidth() != image.getWidth() / averaging )
        || ( averageImage.getHeight() != image.getHeight() / averaging )
      )
    {
#ifdef USE_LOG4CXX
        LOG4CXX_ERROR( loggerTransformation, "size missmatch between original image and average image" );
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
    LOG4CXX_INFO( loggerTransformation, "determine variance ... " );
#endif //USE_LOG4CXX

    unsigned char * const plane0New = &imageBufferNew->image[ 0 ];
    unsigned char * const plane1New = &imageBufferNew->image[ planaImageNew.planeSize0 ];
    unsigned char * const plane2New = &imageBufferNew->image[ planaImageNew.planeSize0 + planaImageNew.planeSize1 ];

    const unsigned char * const plane0Old = &imageBufferOld->image[ 0 ];
    const unsigned char * const plane1Old = &imageBufferOld->image[ planaImageOld.planeSize0 ];
    const unsigned char * const plane2Old = &imageBufferOld->image[ planaImageOld.planeSize0 + planaImageOld.planeSize1 ];

    const unsigned char * const plane0Avg = &imageAvgBuffer->image[ 0 ];
    const unsigned char * const plane1Avg = &imageAvgBuffer->image[ planaImageNew.planeSize0 ];
    const unsigned char * const plane2Avg = &imageAvgBuffer->image[ planaImageNew.planeSize0 + planaImageNew.planeSize1 ];

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

                    const int xNewByteOffset = bytesPerPixel * xNew;
                    const int yNewByteOffset = bytesPerNewLine * yNew;

                    for( int yOldOffset = 0; yOldOffset < averaging; ++yOldOffset )
                    {
                        const int yOld = yNew * averaging + yOldOffset;

                        for( int xOldOffset = 0; xOldOffset < averaging; ++xOldOffset )
                        {
                            const int xOld = xNew * averaging + xOldOffset;

                            const int xOldByteOffset = bytesPerPixel * xOld;
                            const int yOldByteOffset = bytesPerOldLine * yOld;

                            int value  = plane0Old[ xOldByteOffset + yOldByteOffset ];
                            value     -= plane0Avg[ xNewByteOffset + yNewByteOffset ];
                            sum       += ( value * value );
                        }
                    }

                    const int avgAvg = averaging * averaging;
                    plane0New[ xNewByteOffset + yNewByteOffset ] = sum / avgAvg;
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

                    const int xNewByteOffset = bytesPerPixel * xNew;
                    const int yNewByteOffset = bytesPerNewLine * yNew;

                    for( int yOldOffset = 0; yOldOffset < chromaAveragingY; ++yOldOffset )
                    {
                        const int yOld = yNew * chromaAveragingY + yOldOffset;

                        for( int xOldOffset = 0; xOldOffset < chromaAveragingX; ++xOldOffset )
                        {
                            const int xOld = xNew * chromaAveragingX + xOldOffset;

                            const int xOldByteOffset = bytesPerPixel * xOld;
                            const int yOldByteOffset = bytesPerOldLine * yOld;

                            int value  = plane1Old[ xOldByteOffset + yOldByteOffset ];
                            value     -= plane1Avg[ xNewByteOffset + yNewByteOffset ];
                            sum       += ( value * value );
                        }
                    }

                    const int avgAvg = chromaAveragingX * chromaAveragingY;
                    plane1New[ xNewByteOffset + yNewByteOffset ] = sum / avgAvg;
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

                    const int xNewByteOffset = bytesPerPixel * xNew;
                    const int yNewByteOffset = bytesPerNewLine * yNew;

                    for( int yOldOffset = 0; yOldOffset < chromaAveragingY; ++yOldOffset )
                    {
                        const int yOld = yNew * chromaAveragingY + yOldOffset;

                        for( int xOldOffset = 0; xOldOffset < chromaAveragingX; ++xOldOffset )
                        {
                            const int xOld = xNew * chromaAveragingX + xOldOffset;

                            const int xOldByteOffset = bytesPerPixel * xOld;
                            const int yOldByteOffset = bytesPerOldLine * yOld;

                            int value  = plane2Old[ xOldByteOffset + yOldByteOffset ];
                            value     -= plane2Avg[ xNewByteOffset + yNewByteOffset ];
                            sum       += ( value * value );
                        }
                    }

                    const int avgAvg = chromaAveragingX * chromaAveragingY;
                    plane2New[ xNewByteOffset + yNewByteOffset ] = sum / avgAvg;
                }
            }
        }
    }

#ifdef USE_LOG4CXX
    LOG4CXX_INFO( loggerTransformation, "determine variance ... done" );
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
