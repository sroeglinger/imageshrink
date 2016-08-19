
// include system headers
// ...

// include own headers
#include "ImageCovariance.h"

// include application headers
#include "PlanarImageCalc.h"

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
            LOG4CXX_ERROR( loggerTransformation, "unknown pixelformat " << PixelFormat::toString( image1.getPixelFormat() ) );
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

    // check colorspace
    if(    ( image1.getColorspace() != Colorspace::RGB )
        && ( image2.getColorspace() != Colorspace::RGB )
        && ( averageImage1.getColorspace() != Colorspace::RGB )
        && ( averageImage2.getColorspace() != Colorspace::RGB )
      )
    {
        LOG4CXX_ERROR( loggerTransformation, "colorspace is not RGB" );
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
        LOG4CXX_ERROR( loggerTransformation, "pixel-format is not RGB" );
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
        LOG4CXX_ERROR( loggerTransformation, "buffer size mismatch (" << __FILE__ << ", " << __LINE__ << ")" );
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

    ImageBufferShrdPtr newImageBuffer = std::make_shared<ImageBuffer>( newBufferSize );

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

ImageCovariance ImageCovariance::calcCovarianceImage_YUV( const ImageInterface & image1, const ImageInterface & averageImage1, const ImageInterface & image2, const ImageInterface & averageImage2 )
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

    // check colorspace
    if(    ( image1.getColorspace() != Colorspace::YCbCr )
        && ( image2.getColorspace() != Colorspace::YCbCr )
        && ( averageImage1.getColorspace() != Colorspace::YCbCr )
        && ( averageImage2.getColorspace() != Colorspace::YCbCr )
      )
    {
        LOG4CXX_ERROR( loggerTransformation, "colorspace is not YCbCr" );
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
        LOG4CXX_ERROR( loggerTransformation, "pixel-format is not YCbCr_Planar" );
        ret.reset();
        return ret;
    }

    // preparation
    const ChrominanceSubsampling::VALUE cs = image1.getChrominanceSubsampling();

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
            LOG4CXX_ERROR( loggerTransformation, "not supported chrominance subsampling " << ChrominanceSubsampling::toString( cs ) );
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
        LOG4CXX_ERROR( loggerTransformation, "chrominance subsampling mismatch" );
        ret.reset();
        return ret;
    }

    // preparation
    const int oldWidth  = image1.getWidth();
    const int oldHeight = image1.getHeight();
    
    const int newWidth  = oldWidth / averaging;
    const int newHeight = oldHeight / averaging;

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
        LOG4CXX_ERROR( loggerTransformation, "imageBuffer is a nullptr" );
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

    LOG4CXX_INFO( loggerTransformation, "determine convariance ..." );

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

                    for( int yOldOffset = 0; yOldOffset < averaging; ++yOldOffset )
                    {
                        const int yOld = yNew * averaging + yOldOffset;

                        for( int xOldOffset = 0; xOldOffset < averaging; ++xOldOffset )
                        {
                            const int xOld = xNew * averaging + xOldOffset;

                            const int xOldByteOffset = bytesPerPixel * xOld;
                            const int yOldByteOffset = bytesPerOldLine * yOld;

                            int ch1_1  = plane0Image1Old[ xOldByteOffset + yOldByteOffset ];
                            ch1_1     -= plane0AverageImage1Old[ xNewByteOffset + yNewByteOffset ];
                            int ch1_2  = plane0Image2Old[ xOldByteOffset + yOldByteOffset ];
                            ch1_2     -= plane0AverageImage2Old[ xNewByteOffset + yNewByteOffset ];
                            sum       += ch1_1 * ch1_2;
                        }
                    }

                    plane0New[ xNewByteOffset + yNewByteOffset ] = sum / ( averaging * averaging );
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

    LOG4CXX_INFO( loggerTransformation, "determine convariance ... done " );

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
