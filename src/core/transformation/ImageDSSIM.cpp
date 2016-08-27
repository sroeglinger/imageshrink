
// include system headers
#include <cmath>

// include own headers
#include "ImageDSSIM.h"

// include application headers
#include "PlanarImageCalc.h"
#include "ImageCovariance.h"

// include 3rd party headers
#include <log4cxx/logger.h>

namespace imageshrink
{

static log4cxx::LoggerPtr loggerTransformation ( log4cxx::Logger::getLogger( "transformation" ) );

ImageDSSIM::ImageDSSIM()
: m_pixelFormat( PixelFormat::UNKNOWN )
, m_colorspace( Colorspace::UNKNOWN  )
, m_bitsPerPixelAndChannel( BitsPerPixelAndChannel::UNKNOWN )
, m_chrominanceSubsampling( ChrominanceSubsampling::UNKNOWN )
, m_imageBuffer()
, m_width( 0 )
, m_height( 0 )
, m_dssim( 0.0 )
, m_dssimPeak( 0.0 )
, m_dssimValid( false )
{
    reset();
}

ImageDSSIM::ImageDSSIM( const ImageCollection & imageCollection1, const ImageCollection & imageCollection2 )
: m_pixelFormat( PixelFormat::UNKNOWN )
, m_colorspace( Colorspace::UNKNOWN  )
, m_bitsPerPixelAndChannel( BitsPerPixelAndChannel::UNKNOWN )
, m_chrominanceSubsampling( ChrominanceSubsampling::UNKNOWN )
, m_imageBuffer()
, m_width( 0 )
, m_height( 0 )
, m_dssim( 0.0 )
, m_dssimPeak( 0.0 )
, m_dssimValid( false )
{
    reset();
    ImageDSSIM dssim;

    ImageInterfaceShrdPtr image1Original = imageCollection1.getImage( "original" );

    if( image1Original )
    {
        switch( image1Original->getPixelFormat() )
        {
            case PixelFormat::RGB:
                dssim = calcDSSIMImage_RGB( imageCollection1, imageCollection2 );
                break;

            case PixelFormat::YCbCr_Planar:
                dssim = calcDSSIMImage_YUV( imageCollection1, imageCollection2 );
                break;

            default:
                LOG4CXX_ERROR( loggerTransformation, "unknown pixelformat " << PixelFormat::toString( image1Original->getPixelFormat() ) );
                break;
        }

        m_pixelFormat            = dssim.m_pixelFormat;
        m_colorspace             = dssim.m_colorspace;
        m_bitsPerPixelAndChannel = dssim.m_bitsPerPixelAndChannel;
        m_chrominanceSubsampling = dssim.m_chrominanceSubsampling;
        m_imageBuffer            = dssim.m_imageBuffer;
        m_width                  = dssim.m_width;
        m_height                 = dssim.m_height;
        m_dssim                  = dssim.m_dssim;
        m_dssimPeak              = dssim.m_dssimPeak;
        m_dssimValid             = dssim.m_dssimValid;
    }
    else
    {
        LOG4CXX_ERROR( loggerTransformation, "There is no image called 'original' in imageCollection1" );
    }
}

void ImageDSSIM::reset()
{
    m_pixelFormat = PixelFormat::UNKNOWN;
    m_colorspace = Colorspace::UNKNOWN;
    m_bitsPerPixelAndChannel = BitsPerPixelAndChannel::UNKNOWN;
    m_chrominanceSubsampling = ChrominanceSubsampling::UNKNOWN;
    m_imageBuffer.reset();
    m_width = 0;
    m_height = 0;
    m_dssim = 0.0;
    m_dssimPeak = 0.0;
    m_dssimValid = 0.0;
}

ImageDSSIM ImageDSSIM::calcDSSIMImage_RGB( const ImageCollection & imageCollection1, const ImageCollection & imageCollection2 )
{
    ImageDSSIM ret;
    // const int averaging = 8;

    // collect buffers
    ImageInterfaceShrdPtr image1Original = imageCollection1.getImage( "original" );
    ImageInterfaceShrdPtr image1Average  = imageCollection1.getImage( "average" );
    ImageInterfaceShrdPtr image1Variance = imageCollection1.getImage( "variance" );

    ImageInterfaceShrdPtr image2Original = imageCollection2.getImage( "original" );
    ImageInterfaceShrdPtr image2Average  = imageCollection2.getImage( "average" );
    ImageInterfaceShrdPtr image2Variance = imageCollection2.getImage( "variance" );

    // check pointer
    if(    ( !image1Original )
        || ( !image1Average )
        || ( !image1Variance )
        || ( !image2Original )
        || ( !image2Average )
        || ( !image2Variance )
       )
    {
        LOG4CXX_ERROR( loggerTransformation, "at least one image is missing" );
        ret.reset();
        return ret;
    }

    // check colorspace
    if(    ( image1Original->getColorspace() != Colorspace::RGB )
        || ( image1Average->getColorspace()  != Colorspace::RGB )
        || ( image1Variance->getColorspace() != Colorspace::RGB )
        || ( image2Original->getColorspace() != Colorspace::RGB )
        || ( image2Average->getColorspace()  != Colorspace::RGB )
        || ( image2Variance->getColorspace() != Colorspace::RGB )
       )
    {
        LOG4CXX_ERROR( loggerTransformation, "at least one colorspace is not RGB" );
        ret.reset();
        return ret;
    }

    // check pixel format
    if(    ( image1Original->getPixelFormat() != PixelFormat::RGB )
        || ( image1Average->getPixelFormat()  != PixelFormat::RGB )
        || ( image1Variance->getPixelFormat() != PixelFormat::RGB )
        || ( image2Original->getPixelFormat() != PixelFormat::RGB )
        || ( image2Average->getPixelFormat()  != PixelFormat::RGB )
        || ( image2Variance->getPixelFormat() != PixelFormat::RGB )
       )
    {
        LOG4CXX_ERROR( loggerTransformation, "at least one pixel-format is not RGB" );
        ret.reset();
        return ret;
    }

    // check sizes
    if(    ( image1Original->getWidth() != image2Original->getWidth() )
        || ( image1Original->getHeight() != image2Original->getHeight() )

        || ( image1Average->getWidth() != image2Average->getWidth() )
        || ( image1Average->getHeight() != image2Average->getHeight() )

        || ( image1Variance->getWidth() != image2Variance->getWidth() )
        || ( image1Variance->getHeight() != image2Variance->getHeight() )
      )
    {
        LOG4CXX_ERROR( loggerTransformation, "size mismatch between images (" << __FILE__ << ", " << __LINE__ << ")" );
        ret.reset();
        return ret;
    }

    // collect buffers
    ImageBufferShrdPtr image1OriginalBuffer = image1Original->getImageBuffer();
    ImageBufferShrdPtr image1AverageBuffer  = image1Average->getImageBuffer();
    ImageBufferShrdPtr image1VarianceBuffer = image1Variance->getImageBuffer();

    ImageBufferShrdPtr image2OriginalBuffer = image2Original->getImageBuffer();
    ImageBufferShrdPtr image2AverageBuffer  = image2Average->getImageBuffer();
    ImageBufferShrdPtr image2VarianceBuffer = image2Variance->getImageBuffer();

    // check buffers
    if(    ( !image1OriginalBuffer )
        || ( !image1AverageBuffer )
        || ( !image1VarianceBuffer )
        || ( !image2OriginalBuffer )
        || ( !image2AverageBuffer )
        || ( !image2VarianceBuffer )
      )
    {
        LOG4CXX_ERROR( loggerTransformation, "at least one imageBuffer is a nullptr" );
        ret.reset();
        return ret;
    }

    // check sizes
    if(    ( image1OriginalBuffer->size != image2OriginalBuffer->size )
        || ( image1AverageBuffer->size != image2AverageBuffer->size )
        || ( image1VarianceBuffer->size != image2VarianceBuffer->size )
      )
    {
        LOG4CXX_ERROR( loggerTransformation, "size mismatch between images (" << __FILE__ << ", " << __LINE__ << ")" );
        ret.reset();
        return ret;
    }

    // determine the covariance
    ImageCovariance covariance( image1Original, image1Average, image2Original, image2Average );
    ImageBufferShrdPtr covarianceBuffer = covariance.getImageBuffer();

    if( !covarianceBuffer )
    {
        LOG4CXX_ERROR( loggerTransformation, "at least one imageBuffer is a nullptr" );
        ret.reset();
        return ret;
    }

    // check sizes
    if(    ( covariance.getWidth() != image1Average->getWidth() )
        || ( covariance.getHeight() != image1Average->getHeight() )
      )
    {
        LOG4CXX_ERROR( loggerTransformation, "size mismatch between images (" << __FILE__ << ", " << __LINE__ << ")" );
        ret.reset();
        return ret;
    }

    // constants for SSIM
    const double ssimL  = 255;   // 2**(#bits per pixel) - 1
    const double ssimK1 = 0.01;
    const double ssimK2 = 0.03;
    const double ssimC1 = pow( ssimK1 * ssimL, 2.0 );
    const double ssimC2 = pow( ssimK2 * ssimL, 2.0 );

    // determine SSIM
    const int bytesPerPixel = PixelFormat::channelsPerPixel(image1Average->getPixelFormat()) * BitsPerPixelAndChannel::bytesPerChannel(image1Average->getBitsPerPixelAndChannel());
    const int nofPixels     = image1Average->getWidth() * image1Average->getHeight();
    const int bufferSize    = bytesPerPixel * nofPixels;

    const int width  = image1Average->getWidth();
    const int height = image1Average->getHeight();

    ImageBufferShrdPtr newImageBuffer = std::make_shared<ImageBuffer>( bufferSize );

    LOG4CXX_INFO( loggerTransformation, "calculate SSIM ..." );

    const int bytesPerLine = width * bytesPerPixel;
    double dssimSum = 0.0;
    double dssimPeak = -1.0;

    #pragma omp parallel for reduction(+:dssimSum) reduction(max:dssimPeak)
    for( int y = 0; y < height; ++y )
    {
        double dssimLineSum = 0.0;

        for( int x = 0; x < width; ++x )
        {
            const int xByteOffset = bytesPerPixel * x;
            const int yByteOffset = bytesPerLine * y;



            const double averaging1Pixel = image1AverageBuffer->image[ xByteOffset + yByteOffset + 0 ] / ssimL;
            const double variance1Pixel  = image1VarianceBuffer->image[ xByteOffset + yByteOffset + 0 ] / ssimL;

            const double averaging2Pixel = image2AverageBuffer->image[ xByteOffset + yByteOffset + 0 ] / ssimL;
            const double variance2Pixel  = image2VarianceBuffer->image[ xByteOffset + yByteOffset + 0 ] / ssimL;

            const double covariancePixel = covarianceBuffer->image[ xByteOffset + yByteOffset + 0 ] / ssimL;

            const double ssim = ( ( 2.0 * averaging1Pixel * averaging2Pixel + ssimC1 ) * ( 2.0 * covariancePixel + ssimC2 ) )
                                /
                                ( ( averaging1Pixel * averaging1Pixel + averaging2Pixel * averaging2Pixel + ssimC1 ) * ( variance1Pixel + variance2Pixel + ssimC2 ) );

            const double dssim = ( 1.0 - ssim ) / 2.0;

            newImageBuffer->image[ xByteOffset + yByteOffset + 0 ] = dssim * ssimL;
            dssimLineSum += dssim;

            if( dssim > dssimPeak ) 
                dssimPeak = dssim;
        }

        dssimSum += ( dssimLineSum / static_cast<double>(width) );
    }

    LOG4CXX_INFO( loggerTransformation, "calculate SSIM ... done" );

    // collect data
    ret.m_pixelFormat            = image1Average->getPixelFormat();
    ret.m_colorspace             = image1Average->getColorspace();
    ret.m_bitsPerPixelAndChannel = image1Average->getBitsPerPixelAndChannel();
    ret.m_chrominanceSubsampling = image1Average->getChrominanceSubsampling();
    ret.m_imageBuffer            = newImageBuffer;
    ret.m_width                  = width;
    ret.m_height                 = height;
    ret.m_dssim                  = dssimSum / static_cast<double>(height);
    ret.m_dssimPeak              = dssimPeak;
    ret.m_dssimValid             = true;

    return ret;
}

ImageDSSIM ImageDSSIM::calcDSSIMImage_YUV( const ImageCollection & imageCollection1, const ImageCollection & imageCollection2 )
{
    ImageDSSIM ret;
    // const int averaging = 8;

    // collect buffers
    ImageInterfaceShrdPtr image1Original = imageCollection1.getImage( "original" );
    ImageInterfaceShrdPtr image1Average  = imageCollection1.getImage( "average" );
    ImageInterfaceShrdPtr image1Variance = imageCollection1.getImage( "variance" );

    ImageInterfaceShrdPtr image2Original = imageCollection2.getImage( "original" );
    ImageInterfaceShrdPtr image2Average  = imageCollection2.getImage( "average" );
    ImageInterfaceShrdPtr image2Variance = imageCollection2.getImage( "variance" );

    // check pointer
    if(    ( !image1Original )
        || ( !image1Average )
        || ( !image1Variance )
        || ( !image2Original )
        || ( !image2Average )
        || ( !image2Variance )
       )
    {
        LOG4CXX_ERROR( loggerTransformation, "at least one image is missing" );
        ret.reset();
        return ret;
    }

    // check colorspace
    if(    ( image1Original->getColorspace() != Colorspace::YCbCr )
        || ( image1Average->getColorspace()  != Colorspace::YCbCr )
        || ( image1Variance->getColorspace() != Colorspace::YCbCr )
        || ( image2Original->getColorspace() != Colorspace::YCbCr )
        || ( image2Average->getColorspace()  != Colorspace::YCbCr )
        || ( image2Variance->getColorspace() != Colorspace::YCbCr )
       )
    {
        LOG4CXX_ERROR( loggerTransformation, "at least one colorspace is not YCbCr" );
        ret.reset();
        return ret;
    }

    // check pixel format
    if(    ( image1Original->getPixelFormat() != PixelFormat::YCbCr_Planar )
        || ( image1Average->getPixelFormat()  != PixelFormat::YCbCr_Planar )
        || ( image1Variance->getPixelFormat() != PixelFormat::YCbCr_Planar )
        || ( image2Original->getPixelFormat() != PixelFormat::YCbCr_Planar )
        || ( image2Average->getPixelFormat()  != PixelFormat::YCbCr_Planar )
        || ( image2Variance->getPixelFormat() != PixelFormat::YCbCr_Planar )
       )
    {
        LOG4CXX_ERROR( loggerTransformation, "at least one pixel-format is not YCbCr_Planar" );
        ret.reset();
        return ret;
    }

    // check sizes
    if(    ( image1Original->getWidth() != image2Original->getWidth() )
        || ( image1Original->getHeight() != image2Original->getHeight() )

        || ( image1Average->getWidth() != image2Average->getWidth() )
        || ( image1Average->getHeight() != image2Average->getHeight() )

        || ( image1Variance->getWidth() != image2Variance->getWidth() )
        || ( image1Variance->getHeight() != image2Variance->getHeight() )
      )
    {
        LOG4CXX_ERROR( loggerTransformation, "size mismatch between images (" << __FILE__ << ", " << __LINE__ << ")" );
        ret.reset();
        return ret;
    }

    // collect buffers
    ImageBufferShrdPtr image1OriginalBuffer = image1Original->getImageBuffer();
    ImageBufferShrdPtr image1AverageBuffer  = image1Average->getImageBuffer();
    ImageBufferShrdPtr image1VarianceBuffer = image1Variance->getImageBuffer();

    ImageBufferShrdPtr image2OriginalBuffer = image2Original->getImageBuffer();
    ImageBufferShrdPtr image2AverageBuffer  = image2Average->getImageBuffer();
    ImageBufferShrdPtr image2VarianceBuffer = image2Variance->getImageBuffer();

    // check buffers
    if(    ( !image1OriginalBuffer )
        || ( !image1AverageBuffer )
        || ( !image1VarianceBuffer )
        || ( !image2OriginalBuffer )
        || ( !image2AverageBuffer )
        || ( !image2VarianceBuffer )
      )
    {
        LOG4CXX_ERROR( loggerTransformation, "at least one imageBuffer is a nullptr" );
        ret.reset();
        return ret;
    }

    // check sizes
    if(    ( image1OriginalBuffer->size != image2OriginalBuffer->size )
        || ( image1AverageBuffer->size != image2AverageBuffer->size )
        || ( image1VarianceBuffer->size != image2VarianceBuffer->size )
      )
    {
        LOG4CXX_ERROR( loggerTransformation, "size mismatch between images (" << __FILE__ << ", " << __LINE__ << ")" );

        LOG4CXX_ERROR( loggerTransformation, "image1OriginalBuffer->size: " << image1OriginalBuffer->size );
        LOG4CXX_ERROR( loggerTransformation, "image1AverageBuffer->size:  " << image1AverageBuffer->size );
        LOG4CXX_ERROR( loggerTransformation, "image1VarianceBuffer->size: " << image1VarianceBuffer->size );
        
        LOG4CXX_ERROR( loggerTransformation, "image2OriginalBuffer->size: " << image2OriginalBuffer->size );
        LOG4CXX_ERROR( loggerTransformation, "image2AverageBuffer->size:  " << image2AverageBuffer->size );
        LOG4CXX_ERROR( loggerTransformation, "image2VarianceBuffer->size: " << image2VarianceBuffer->size );

        ret.reset();
        return ret;
    }

    // determine the covariance
    ImageCovariance covariance( image1Original, image1Average, image2Original, image2Average );
    ImageBufferShrdPtr covarianceBuffer = covariance.getImageBuffer();

    if( !covarianceBuffer )
    {
        LOG4CXX_ERROR( loggerTransformation, "at least one imageBuffer is a nullptr" );
        ret.reset();
        return ret;
    }
    
    // constants for SSIM
    const double ssimL  = 255;   // 2**(#bits per pixel) - 1
    const double ssimK1 = 0.01;
    const double ssimK2 = 0.03;
    const double ssimC1 = pow( ssimK1 * ssimL, 2.0 );
    const double ssimC2 = pow( ssimK2 * ssimL, 2.0 );

    // preparation
    const ChrominanceSubsampling::VALUE cs = image1Average->getChrominanceSubsampling();

    const int width  = image1Average->getWidth();
    const int height = image1Average->getHeight();

    PlanarImageDesc planaImageNew = calcPlanaerImageDescForYUV( width, height, cs, TJ_PAD );

    ImageBufferShrdPtr imageBufferNew = std::make_shared<ImageBuffer>( planaImageNew.bufferSize );

    if(    ( !imageBufferNew )
        // || ( !imageBufferOld )
      )
    {
        LOG4CXX_ERROR( loggerTransformation, "imageBuffer is a nullptr" );
        ret.reset();
        return ret;
    }

    // check chrominance subsampling
    if(    ( image1Original->getChrominanceSubsampling() != cs )
        && ( image1Average->getChrominanceSubsampling() != cs )
        && ( image1Variance->getChrominanceSubsampling() != cs )
        && ( image2Original->getChrominanceSubsampling() != cs )
        && ( image2Average->getChrominanceSubsampling() != cs )
        && ( image2Variance->getChrominanceSubsampling() != cs )
      )
    {
        LOG4CXX_ERROR( loggerTransformation, "chrominance subsampling mismatch" );
        ret.reset();
        return ret;
    }

    // preparation
    LOG4CXX_INFO( loggerTransformation, "calculate SSIM ..." );

    const unsigned char * const plane0Image1Average = &image1AverageBuffer->image[ 0 ];
//    const unsigned char * const plane1Image1Average = &image1AverageBuffer->image[ planaImageNew.planeSize0 ];
//    const unsigned char * const plane2Image1Average = &image1AverageBuffer->image[ planaImageNew.planeSize0 + planaImageNew.planeSize1 ];

    const unsigned char * const plane0Image2Average = &image2AverageBuffer->image[ 0 ];
//    const unsigned char * const plane1Image2Average = &image2AverageBuffer->image[ planaImageNew.planeSize0 ];
//    const unsigned char * const plane2Image2Average = &image2AverageBuffer->image[ planaImageNew.planeSize0 + planaImageNew.planeSize1 ];

    const unsigned char * const plane0Image1Variance = &image1VarianceBuffer->image[ 0 ];
//    const unsigned char * const plane1Image1Variance = &image1VarianceBuffer->image[ planaImageNew.planeSize0 ];
//    const unsigned char * const plane2Image1Variance = &image1VarianceBuffer->image[ planaImageNew.planeSize0 + planaImageNew.planeSize1 ];

    const unsigned char * const plane0Image2Variance = &image2VarianceBuffer->image[ 0 ];
//    const unsigned char * const plane1Image2Variance = &image2VarianceBuffer->image[ planaImageNew.planeSize0 ];
//    const unsigned char * const plane2Image2Variance = &image2VarianceBuffer->image[ planaImageNew.planeSize0 + planaImageNew.planeSize1 ];

    const unsigned char * const plane0Covariance = &covarianceBuffer->image[ 0 ];
//    const unsigned char * const plane1Covariance = &covarianceBuffer->image[ planaImageNew.planeSize0 ];
//    const unsigned char * const plane2Covariance = &covarianceBuffer->image[ planaImageNew.planeSize0 + planaImageNew.planeSize1 ];

    unsigned char * const plane0New = &imageBufferNew->image[ 0 ];
//    unsigned char * const plane1New = &imageBufferNew->image[ planaImageNew.planeSize0 ];
//    unsigned char * const plane2New = &imageBufferNew->image[ planaImageNew.planeSize0 + planaImageNew.planeSize1 ];

    double dssimSum = 0.0;
    double dssimPeak = -1.0;

    {
        const int bytesPerLine = planaImageNew.stride0;
        const int bytesPerPixel = 1;
        

        #pragma omp parallel for reduction(+:dssimSum) reduction(max:dssimPeak)
        for( int y = 0; y < height; ++y )
        {
            double dssimLineSum = 0.0;

            for( int x = 0; x < width; ++x )
            {
                const int xByteOffset = bytesPerPixel * x;
                const int yByteOffset = bytesPerLine * y;



                const double averaging1Pixel = plane0Image1Average[ xByteOffset + yByteOffset + 0 ] / ssimL;
                const double variance1Pixel  = plane0Image1Variance[ xByteOffset + yByteOffset + 0 ] / ssimL;

                const double averaging2Pixel = plane0Image2Average[ xByteOffset + yByteOffset + 0 ] / ssimL;
                const double variance2Pixel  = plane0Image2Variance[ xByteOffset + yByteOffset + 0 ] / ssimL;

                const double covariancePixel = plane0Covariance[ xByteOffset + yByteOffset + 0 ] / ssimL;

                const double ssim = ( ( 2.0 * averaging1Pixel * averaging2Pixel + ssimC1 ) * ( 2.0 * covariancePixel + ssimC2 ) )
                                    /
                                    ( ( averaging1Pixel * averaging1Pixel + averaging2Pixel * averaging2Pixel + ssimC1 ) * ( variance1Pixel + variance2Pixel + ssimC2 ) );

                const double dssim = ( 1.0 - ssim ) / 2.0;

                plane0New[ xByteOffset + yByteOffset + 0 ] = dssim * ssimL;
                dssimLineSum += dssim;

                if( dssim > dssimPeak ) 
                    dssimPeak = dssim;
            }

            dssimSum += ( dssimLineSum / static_cast<double>(width) );
        }


    }

    LOG4CXX_INFO( loggerTransformation, "calculate SSIM ... done" );

    // collect data
    ret.m_pixelFormat            = image1Average->getPixelFormat();
    ret.m_colorspace             = image1Average->getColorspace();
    ret.m_bitsPerPixelAndChannel = image1Average->getBitsPerPixelAndChannel();
    ret.m_chrominanceSubsampling = image1Average->getChrominanceSubsampling();
    ret.m_imageBuffer            = imageBufferNew;
    ret.m_width                  = width;
    ret.m_height                 = height;
    ret.m_dssim                  = dssimSum / static_cast<double>(height);
    ret.m_dssimPeak              = dssimPeak;
    ret.m_dssimValid             = true;


    return ret;
}

double ImageDSSIM::getDssim()
{
    if( !m_dssimValid )
    {
        LOG4CXX_FATAL( loggerTransformation, "dssim value is NOT valid!!!" );
    }

    return m_dssim;
}

double ImageDSSIM::getDssimPeak()
{
    if( !m_dssimValid )
    {
        LOG4CXX_FATAL( loggerTransformation, "dssimPeak value is NOT valid!!!" );
    }
    
    return m_dssimPeak;
}

} //namespace imageshrink
