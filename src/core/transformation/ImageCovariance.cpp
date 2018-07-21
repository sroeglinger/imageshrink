
// include system headers
// ...

// include own headers
#include "ImageCovariance.h"

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

ImageCovariance::ImageCovariance()
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

ImageCovariance::ImageCovariance( ImageInterfaceShrdPtr image1, ImageInterfaceShrdPtr averageImage1, ImageInterfaceShrdPtr image2, ImageInterfaceShrdPtr averageImage2, int averaging )
: ImageCovariance( *image1, *averageImage1, *image2, *averageImage2, averaging )
{
    // nothing
}

ImageCovariance::ImageCovariance( const ImageInterface & image1, const ImageInterface & averageImage1, const ImageInterface & image2, const ImageInterface & averageImage2, int averaging )
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
    ImageCovariance var;

    switch( image1.getPixelFormat() )
    {
        case PixelFormat::RGB:
            var = calcCovarianceImage_RGB( image1, averageImage1, image2, averageImage2 );
            break;

        case PixelFormat::YCbCr_Planar:
            var = calcCovarianceImage_YUV( image1, averageImage1, image2, averageImage2 );
            break;

        default:
#ifdef USE_LOG4CXX
            LOG4CXX_ERROR( loggerTransformation, "unknown pixelformat " << PixelFormat::toString( image1.getPixelFormat() ) );
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

void ImageCovariance::reset()
{
    m_pixelFormat = PixelFormat::UNKNOWN;
    m_colorspace = Colorspace::UNKNOWN;
    m_bitsPerPixelAndChannel = BitsPerPixelAndChannel::UNKNOWN;
    m_chrominanceSubsampling = ChrominanceSubsampling::UNKNOWN;
    m_imageBuffer.reset();
}

ImageCovariance ImageCovariance::calcCovarianceImage_RGB( const ImageInterface & image1, const ImageInterface & averageImage1, const ImageInterface & image2, const ImageInterface & averageImage2 )
{
    ImageCovariance ret;

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
#ifdef USE_LOG4CXX
        LOG4CXX_ERROR( loggerTransformation, "imageBuffer is a nullptr" );
#endif //USE_LOG4CXX
        ret.reset();
        return ret;
    }

    if(    ( image1.getWidth() != image2.getWidth() )
        || ( image1.getHeight() != image2.getHeight() )
        || ( averageImage1.getWidth() != averageImage2.getWidth() )
        || ( averageImage1.getHeight() != averageImage2.getHeight() )
        || ( averageImage1.getWidth() != image1.getWidth() / m_averaging )
        || ( averageImage1.getHeight() != image1.getHeight() / m_averaging )
      )
    {
#ifdef USE_LOG4CXX
        LOG4CXX_ERROR( loggerTransformation, "size missmatch between images" );
#endif //USE_LOG4CXX
        ret.reset();
        return ret;
    }

    // check colorspace
    if(    ( image1.getColorspace() != Colorspace::RGB )
        && ( image2.getColorspace() != Colorspace::RGB )
        && ( averageImage1.getColorspace() != Colorspace::RGB )
        && ( averageImage2.getColorspace() != Colorspace::RGB )
      )
    {
#ifdef USE_LOG4CXX
        LOG4CXX_ERROR( loggerTransformation, "colorspace is not RGB" );
#endif //USE_LOG4CXX
        ret.reset();
        return ret;
    }

    // check pixel format
    if(    ( image1.getPixelFormat() != PixelFormat::RGB )
        && ( image2.getPixelFormat() != PixelFormat::RGB )
        && ( averageImage1.getPixelFormat() != PixelFormat::RGB )
        && ( averageImage2.getPixelFormat() != PixelFormat::RGB )
      )
    {
#ifdef USE_LOG4CXX
        LOG4CXX_ERROR( loggerTransformation, "pixel-format is not RGB" );
#endif //USE_LOG4CXX
        ret.reset();
        return ret;
    }

    // determine buffer size and check it
    const int bytesPerPixel = PixelFormat::channelsPerPixel(image1.getPixelFormat()) * BitsPerPixelAndChannel::bytesPerChannel(image1.getBitsPerPixelAndChannel());
    const int nofPixels     = image1.getWidth() * image1.getHeight();
    const int bufferSize    = bytesPerPixel * nofPixels;

    if(    ( bufferSize != imageBuffer1->size )
        || ( bufferSize != imageBuffer2->size )
      )
    {
#ifdef USE_LOG4CXX
        LOG4CXX_ERROR( loggerTransformation, "buffer size mismatch (" << __FILE__ << ", " << __LINE__ << ")" );
#endif //USE_LOG4CXX
        ret.reset();
        return ret;
    }

    // determine new image
    const int newWidth  = image1.getWidth() / m_averaging;
    const int newHeight = image1.getHeight() / m_averaging;

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

    int bytesPerLine = image1.getWidth() * bytesPerPixel;
    int bytesPerNewLine = bytesPerLine / m_averaging;

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

            for( int xWindow = 0; xWindow < m_averaging; ++xWindow )
            {
                const int xWindowByteOffset = bytesPerPixel * ( xNew * m_averaging + xWindow );

                for( int yWindow = 0; yWindow < m_averaging; ++yWindow )
                {
                    const int yWindowByteOffset = bytesPerLine * ( yNew * m_averaging + yWindow );

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

            const int avgAvg = m_averaging * m_averaging;
            newImageBuffer->image[ xNewByteOffset + yNewByteOffset + 0 ] = sumCh1 / avgAvg;
            newImageBuffer->image[ xNewByteOffset + yNewByteOffset + 1 ] = sumCh2 / avgAvg;
            newImageBuffer->image[ xNewByteOffset + yNewByteOffset + 2 ] = sumCh3 / avgAvg;
        }
    }

#ifdef USE_LOG4CXX
    LOG4CXX_INFO( loggerTransformation, "averaging ... done" );
#endif //USE_LOG4CXX

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

ImageCovariance ImageCovariance::calcCovarianceImage_YUV( const ImageInterface & image1, const ImageInterface & averageImage1, const ImageInterface & image2, const ImageInterface & averageImage2 )
{
    ImageCovariance ret;

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
#ifdef USE_LOG4CXX
        LOG4CXX_ERROR( loggerTransformation, "imageBuffer is a nullptr" );
#endif //USE_LOG4CXX
        ret.reset();
        return ret;
    }

    if(    ( image1.getWidth() != image2.getWidth() )
        || ( image1.getHeight() != image2.getHeight() )
        || ( averageImage1.getWidth() != averageImage2.getWidth() )
        || ( averageImage1.getHeight() != averageImage2.getHeight() )
        || ( averageImage1.getWidth() != image1.getWidth() / m_averaging )
        || ( averageImage1.getHeight() != image1.getHeight() / m_averaging )
      )
    {
#ifdef USE_LOG4CXX
        LOG4CXX_ERROR( loggerTransformation, "size missmatch between images" );
#endif //USE_LOG4CXX
        ret.reset();
        return ret;
    }

    // check colorspace
    if(    ( image1.getColorspace() != Colorspace::YCbCr )
        && ( image2.getColorspace() != Colorspace::YCbCr )
        && ( averageImage1.getColorspace() != Colorspace::YCbCr )
        && ( averageImage2.getColorspace() != Colorspace::YCbCr )
      )
    {
#ifdef USE_LOG4CXX
        LOG4CXX_ERROR( loggerTransformation, "colorspace is not YCbCr" );
#endif //USE_LOG4CXX
        ret.reset();
        return ret;
    }

    // check pixel format
    if(    ( image1.getPixelFormat() != PixelFormat::YCbCr_Planar )
        && ( image2.getPixelFormat() != PixelFormat::YCbCr_Planar )
        && ( averageImage1.getPixelFormat() != PixelFormat::YCbCr_Planar )
        && ( averageImage2.getPixelFormat() != PixelFormat::YCbCr_Planar )
      )
    {
#ifdef USE_LOG4CXX
        LOG4CXX_ERROR( loggerTransformation, "pixel-format is not YCbCr_Planar" );
#endif //USE_LOG4CXX
        ret.reset();
        return ret;
    }

    // preparation
    const ChrominanceSubsampling::VALUE cs = image1.getChrominanceSubsampling();

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

    // check chrominance subsampling
    if(    ( image1.getChrominanceSubsampling() != cs )
        && ( image2.getChrominanceSubsampling() != cs )
        && ( averageImage1.getChrominanceSubsampling() != cs )
        && ( averageImage2.getChrominanceSubsampling() != cs )
      )
    {
#ifdef USE_LOG4CXX
        LOG4CXX_ERROR( loggerTransformation, "chrominance subsampling mismatch" );
#endif //USE_LOG4CXX
        ret.reset();
        return ret;
    }

    // preparation
    const int oldWidth  = image1.getWidth();
    const int oldHeight = image1.getHeight();
    
    const int newWidth  = oldWidth / m_averaging;
    const int newHeight = oldHeight / m_averaging;

    PlanarImageDesc planaImageNew = calcPlanaerImageDescForYUV( newWidth, newHeight, cs, TJ_PAD );
    PlanarImageDesc planaImageOld = calcPlanaerImageDescForYUV( oldWidth, oldHeight, cs, TJ_PAD );

    ImageBufferShrdPtr imageBufferNew = std::make_shared<ImageBuffer>( planaImageNew.bufferSize );
    ImageBufferShrdPtr image1BufferOld = image1.getImageBuffer();
    ImageBufferShrdPtr image2BufferOld = image2.getImageBuffer();
    ImageBufferShrdPtr averageImage1BufferOld = averageImage1.getImageBuffer();
    ImageBufferShrdPtr averageImage2BufferOld = averageImage2.getImageBuffer();

    if(    ( !imageBufferNew )
        || ( !image1BufferOld )
        || ( !image2BufferOld )
        || ( !averageImage1BufferOld )
        || ( !averageImage2BufferOld )
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
    LOG4CXX_INFO( loggerTransformation, "determine convariance ..." );
#endif //USE_LOG4CXX

    unsigned char * const plane0New = &imageBufferNew->image[ 0 ];
    unsigned char * const plane1New = &imageBufferNew->image[ planaImageNew.planeSize0 ];
    unsigned char * const plane2New = &imageBufferNew->image[ planaImageNew.planeSize0 + planaImageNew.planeSize1 ];

    const unsigned char * const plane0Image1Old = &image1BufferOld->image[ 0 ];
    const unsigned char * const plane1Image1Old = &image1BufferOld->image[ planaImageOld.planeSize0 ];
    const unsigned char * const plane2Image1Old = &image1BufferOld->image[ planaImageOld.planeSize0 + planaImageOld.planeSize1 ];

    const unsigned char * const plane0Image2Old = &image2BufferOld->image[ 0 ];
    const unsigned char * const plane1Image2Old = &image2BufferOld->image[ planaImageOld.planeSize0 ];
    const unsigned char * const plane2Image2Old = &image2BufferOld->image[ planaImageOld.planeSize0 + planaImageOld.planeSize1 ];

    const unsigned char * const plane0AverageImage1Old = &averageImage1BufferOld->image[ 0 ];
    const unsigned char * const plane1AverageImage1Old = &averageImage1BufferOld->image[ planaImageNew.planeSize0 ];
    const unsigned char * const plane2AverageImage1Old = &averageImage1BufferOld->image[ planaImageNew.planeSize0 + planaImageNew.planeSize1 ];

    const unsigned char * const plane0AverageImage2Old = &averageImage2BufferOld->image[ 0 ];
    const unsigned char * const plane1AverageImage2Old = &averageImage2BufferOld->image[ planaImageNew.planeSize0 ];
    const unsigned char * const plane2AverageImage2Old = &averageImage2BufferOld->image[ planaImageNew.planeSize0 + planaImageNew.planeSize1 ];

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

                    for( int yOldOffset = 0; yOldOffset < m_averaging; ++yOldOffset )
                    {
                        const int yOld = yNew * m_averaging + yOldOffset;

                        for( int xOldOffset = 0; xOldOffset < m_averaging; ++xOldOffset )
                        {
                            const int xOld = xNew * m_averaging + xOldOffset;

                            const int xOldByteOffset = bytesPerPixel * xOld;
                            const int yOldByteOffset = bytesPerOldLine * yOld;

                            int ch1_1  = plane0Image1Old[ xOldByteOffset + yOldByteOffset ];
                            ch1_1     -= plane0AverageImage1Old[ xNewByteOffset + yNewByteOffset ];
                            int ch1_2  = plane0Image2Old[ xOldByteOffset + yOldByteOffset ];
                            ch1_2     -= plane0AverageImage2Old[ xNewByteOffset + yNewByteOffset ];
                            sum       += ch1_1 * ch1_2;
                        }
                    }

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

                            int ch1_1  = plane1Image1Old[ xOldByteOffset + yOldByteOffset ];
                            ch1_1     -= plane1AverageImage1Old[ xNewByteOffset + yNewByteOffset ];
                            int ch1_2  = plane1Image2Old[ xOldByteOffset + yOldByteOffset ];
                            ch1_2     -= plane1AverageImage2Old[ xNewByteOffset + yNewByteOffset ];
                            sum       += ch1_1 * ch1_2;
                        }
                    }

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

                            int ch1_1  = plane2Image1Old[ xOldByteOffset + yOldByteOffset ];
                            ch1_1     -= plane2AverageImage1Old[ xNewByteOffset + yNewByteOffset ];
                            int ch1_2  = plane2Image2Old[ xOldByteOffset + yOldByteOffset ];
                            ch1_2     -= plane2AverageImage2Old[ xNewByteOffset + yNewByteOffset ];
                            sum       += ch1_1 * ch1_2;
                        }
                    }

                    plane2New[ xNewByteOffset + yNewByteOffset ] = sum / ( chromaAveragingX * chromaAveragingY );
                }
            }
        }
    }

#ifdef USE_LOG4CXX
    LOG4CXX_INFO( loggerTransformation, "determine convariance ... done " );
#endif //USE_LOG4CXX

    // collect information
    ret.m_pixelFormat = image1.getPixelFormat();
    ret.m_colorspace = image1.getColorspace();
    ret.m_bitsPerPixelAndChannel = image1.getBitsPerPixelAndChannel();
    ret.m_chrominanceSubsampling = cs;
    ret.m_imageBuffer = imageBufferNew;
    ret.m_width = newWidth;
    ret.m_height = newHeight;

    return ret;
}

} //namespace imageshrink
