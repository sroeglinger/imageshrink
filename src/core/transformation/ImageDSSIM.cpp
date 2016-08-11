
// include system headers
#include <cmath>

// include own headers
#include "ImageDSSIM.h"
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
    ImageDSSIM dssim = calcDSSIMImage( imageCollection1, imageCollection2 );

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

ImageDSSIM ImageDSSIM::calcDSSIMImage( const ImageCollection & imageCollection1, const ImageCollection & imageCollection2 )
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

    ImageBufferShrdPtr newImageBuffer = ImageBufferShrdPtr( new ImageBuffer( bufferSize ) );

    LOG4CXX_INFO( loggerTransformation, "calculate SSIM ..." );

    int bytesPerLine = width * bytesPerPixel;
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
