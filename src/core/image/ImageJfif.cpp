
// include system headers
#include <fstream>      // std::ifstream
#include <limits>       // std::numeric_limits<...>::...
#include <cstring>      // std::memcpy

// include own headers
#include "ImageJfif.h"

// include application headers
#include "PlanarImageCalc.h"

// include 3rd party headers
#include <turbojpeg.h>
#include <log4cxx/logger.h>

namespace imageshrink
{

static log4cxx::LoggerPtr loggerImage( log4cxx::Logger::getLogger( "image" ) );

ImageJfif::ImageJfif()
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

ImageJfif::ImageJfif( stringConstShrdPtr path )
: ImageJfif( *path )
{
    // nothing
}

ImageJfif::ImageJfif( const std::string & path )
: m_pixelFormat( PixelFormat::UNKNOWN )
, m_colorspace( Colorspace::UNKNOWN  )
, m_bitsPerPixelAndChannel( BitsPerPixelAndChannel::UNKNOWN )
, m_chrominanceSubsampling( ChrominanceSubsampling::UNKNOWN )
, m_imageBuffer()
, m_width( 0 )
, m_height( 0 )
{
    reset();
    loadImage( path );
}

ImageJfif::ImageJfif( const ImageInterface & image )
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
    m_imageBuffer            = image.getImageBuffer();  //TODO: deep copy
    m_width                  = image.getWidth();
    m_height                 = image.getHeight();
}

ImageJfif::ImageJfif( ImageInterfaceShrdPtr image )
: ImageJfif( *image )
{
    // nothing
}

void ImageJfif::reset()
{
    m_pixelFormat = PixelFormat::UNKNOWN;
    m_colorspace = Colorspace::UNKNOWN;
    m_bitsPerPixelAndChannel = BitsPerPixelAndChannel::UNKNOWN;
    m_chrominanceSubsampling = ChrominanceSubsampling::UNKNOWN;
    m_imageBuffer.reset();
}

void ImageJfif::loadImage( const std::string & path )
{
    // int tjRet = 0;

    // open file
    std::ifstream ifs( path.c_str(), std::ifstream::in | std::ifstream::binary );

    // determine size of file
    ifs.ignore( std::numeric_limits<std::streamsize>::max() );
    const std::streamsize length = ifs.gcount();
    ifs.clear();   //  Since ignore will have set eof.
    ifs.seekg( 0, std::ios_base::beg );

    const long unsigned int jpegSize = length;

    // read file
    LOG4CXX_INFO( loggerImage, "read JFIF file with " << length << " Bytes ..." );

    ImageBufferShrdPtr compressedImage = std::make_shared<ImageBuffer>(jpegSize);
    ifs.read( reinterpret_cast<char*>( compressedImage->image ), length );
    ifs.close();

    LOG4CXX_INFO( loggerImage, "read JFIF file with " << length << " Bytes ... done" );
    
    // decompress jpeg
    ImageJfif image = decompress( compressedImage );

    m_pixelFormat            = image.m_pixelFormat;
    m_colorspace             = image.m_colorspace;
    m_bitsPerPixelAndChannel = image.m_bitsPerPixelAndChannel;
    m_chrominanceSubsampling = image.m_chrominanceSubsampling;
    m_imageBuffer            = image.m_imageBuffer;
    m_width                  = image.m_width;
    m_height                 = image.m_height;
}

void ImageJfif::storeInFile( const std::string & path, int quality, ChrominanceSubsampling::VALUE cs )
{    
    if( !m_imageBuffer )
    {
        LOG4CXX_ERROR( loggerImage, "m_imageBuffer is a nullptr" );
        return;
    }

    // compress image
    ImageBufferShrdPtr compressedImage = compress( *this, quality, cs );

    // write new image
    LOG4CXX_INFO( loggerImage, "write to file ..." );

    std::ofstream ofs ( path.c_str(), std::ifstream::out | std::ifstream::binary );
    ofs.write( reinterpret_cast<const char*>( compressedImage->image ), compressedImage->size );
    ofs.close();

    LOG4CXX_INFO( loggerImage, "write to file ... done" );
}

ImageJfif ImageJfif::decompress( ImageBufferShrdPtr compressedImage )
{
    ImageJfif ret;
    int tjRet = 0;

    // check
    if( !compressedImage )
    {
        LOG4CXX_ERROR( loggerImage, "compressedImage is a nullptr" );
        return ret;
    }

    // decompress jpeg
    tjhandle jpegDecompressor = tjInitDecompress();

    int jpegSubsamp, width, height, jpegColorspace;
    tjRet = tjDecompressHeader3(
        jpegDecompressor,
        reinterpret_cast<const unsigned char*>( compressedImage->image ),
        compressedImage->size,
        &width,
        &height,
        &jpegSubsamp,
        &jpegColorspace
    );

    if( tjRet == 0 )
    {
        ret.m_colorspace             = convertTjJpegColorspace( jpegColorspace );
        ret.m_pixelFormat            = PixelFormat::YCbCr_Planar;
        ret.m_bitsPerPixelAndChannel = BitsPerPixelAndChannel::BITS_8;
        ret.m_chrominanceSubsampling = convertTjJpegSubsamp( jpegSubsamp );
        ret.m_width                  = width;
        ret.m_height                 = height;

        LOG4CXX_INFO( loggerImage, "width:  " <<  width << "; height: " <<  height );
        LOG4CXX_INFO( loggerImage, "chrominance subsampling: " << ChrominanceSubsampling::toString( convertTjJpegSubsamp(jpegSubsamp) ) );
        LOG4CXX_INFO( loggerImage, "colorspace: " << Colorspace::toString( ret.m_colorspace ) );
        LOG4CXX_INFO( loggerImage, "pixelformat: " << PixelFormat::toString( ret.m_pixelFormat ) );
        LOG4CXX_INFO( loggerImage, "pits per pixel and channel: " << BitsPerPixelAndChannel::toString( ret.m_bitsPerPixelAndChannel ) );
    }
    else
    {
        LOG4CXX_ERROR( loggerImage, "tjGetErrorStr(): " <<  tjGetErrorStr() );
    }

    // allocate buffer for decompressed image
    const unsigned long yuvPlanarBufferSize = tjBufSizeYUV2( width, /*pad*/ TJ_PAD, height, jpegSubsamp );
    ImageBufferShrdPtr imageBuffer = std::make_shared<ImageBuffer>( yuvPlanarBufferSize );

    // decompress image
    LOG4CXX_INFO( loggerImage, "decompress JFIF image ..." );

    tjRet = tjDecompressToYUV2(
        jpegDecompressor, 
        reinterpret_cast<const unsigned char*>( compressedImage->image ), 
        compressedImage->size, 
        imageBuffer->image,
        width, 
        /*pad*/ TJ_PAD,
        height, 
        // tjOutputPixelFormat, 
        TJFLAG_ACCURATEDCT /*TJFLAG_FASTDCT*/
    );

    LOG4CXX_INFO( loggerImage, "decompress JFIF image ... done" );

    if( tjRet == 0 )
    {
        ret.m_imageBuffer = imageBuffer;
    }
    else
    {
        ret.reset();
        LOG4CXX_INFO( loggerImage, "tjGetErrorStr(): " <<  tjGetErrorStr() );
    }

    tjDestroy(jpegDecompressor);

    return ret;
}

ImageBufferShrdPtr ImageJfif::compress( const ImageJfif & notCompressed, int quality, ChrominanceSubsampling::VALUE cs )
{
    ImageBufferShrdPtr ret;
    const int jpegSubsamp = convert2Tj( cs );
    int tjRet = 0;



    ImageJfif image4Compression = convertChrominanceSubsampling( notCompressed, cs );


    // check image
    if( !image4Compression.m_imageBuffer )
    {
        LOG4CXX_ERROR( loggerImage, "imageBuffer is a nullptr" );
        return ret;
    }

    // compress jpeg
    tjhandle jpegCompressor = tjInitCompress();

    long unsigned int jpegSize = tjBufSize( image4Compression.m_width, image4Compression.m_height, jpegSubsamp );
    unsigned char* compressedImageBuffer = tjAlloc( jpegSize );

    LOG4CXX_INFO( loggerImage, "compress image ..." );

    tjRet = tjCompressFromYUV(
        jpegCompressor,
        reinterpret_cast<const unsigned char*>( image4Compression.m_imageBuffer->image ),
        image4Compression.m_width,
        /*pad*/ TJ_PAD,
        image4Compression.m_height,
        jpegSubsamp,
        &compressedImageBuffer,
        &jpegSize,
        quality,
        TJFLAG_ACCURATEDCT /*TJFLAG_FASTDCT*/
    );

    LOG4CXX_INFO( loggerImage, "compress image ... done" );

    if( tjRet == 0 )
    {
        ret = std::make_shared<ImageBuffer>(jpegSize);
        std::memcpy( ret->image, compressedImageBuffer, jpegSize );
    }
    else
    {
        ret.reset();
        LOG4CXX_INFO( loggerImage, "tjGetErrorStr(): " <<  tjGetErrorStr() );
    }

    tjFree( compressedImageBuffer );
    tjDestroy( jpegCompressor );

    return ret;
}

ImageJfif ImageJfif::convertChrominanceSubsampling( const ImageJfif & image, ChrominanceSubsampling::VALUE cs )
{
    ImageJfif ret;
    bool done = false;
    const ChrominanceSubsampling::VALUE csOld = image.getChrominanceSubsampling();

    // check image
    if( !image.m_imageBuffer )
    {
        LOG4CXX_ERROR( loggerImage, "imageBuffer is a nullptr" );
        return ret;
    }

    // early return if nothing has to be done
    if( csOld == cs )
    {
        return image;
    }

    // change chrominance subsampling
    if( csOld == ChrominanceSubsampling::CS_444 )
    {
        switch( cs )
        {
            case ChrominanceSubsampling::CS_420: ret = convertChrominanceSubsampling_444to420( image ); done = true; break;
            default: 
                /* nothing */ 
                break;
        }
    }
    else if( csOld == ChrominanceSubsampling::CS_420 )
    {
        switch( cs )
        {
            case ChrominanceSubsampling::CS_444: ret = convertChrominanceSubsampling_420to444( image ); done = true; break;
            default: 
                /* nothing */ 
                break;
        }
    }

    if( !done )
    {
        LOG4CXX_ERROR( loggerImage, "chrominance subsampling not converted" );
    }

    return ret;
}

ImageJfif ImageJfif::convertChrominanceSubsampling_444to420( const ImageJfif & image )
{
    ImageJfif ret;

    // check image
    if( !image.m_imageBuffer )
    {
        LOG4CXX_ERROR( loggerImage, "imageBuffer is a nullptr" );
        return ret;
    }

    // preparation
    const int width      = image.getWidth();
    const int height     = image.getHeight();

    PlanarImageDesc planaImageNew = calcPlanaerImageDescForYUV( width, height, ChrominanceSubsampling::CS_420, TJ_PAD );
    PlanarImageDesc planaImageOld = calcPlanaerImageDescForYUV( width, height, ChrominanceSubsampling::CS_444, TJ_PAD );

    ImageBufferShrdPtr imageBufferNew = std::make_shared<ImageBuffer>( planaImageNew.bufferSize );
    ImageBufferShrdPtr imageBufferOld = image.getImageBuffer();

    // if( ( planaImageNew.planeSize0 + planaImageNew.planeSize1 + planaImageNew.planeSize2 ) != planaImageNew.bufferSize )
    // {
    //     LOG4CXX_ERROR( loggerImage, "buffer size mismatch (" << __FILE__ << ", " << __LINE__ << ")" );
    //     return ret;
    // }

    // if( planeSize0New != planeSize0Old )
    // {
    //     LOG4CXX_ERROR( loggerImage, 
    //         "buffersize mismatch; " 
    //         << "planeSize0New ("
    //         << planeSize0New
    //         << ") != planeSize0Old ("
    //         << planeSize0Old
    //         << ")"
    //     );
    //     return ret;
    // }    

    unsigned char * const plane0New = &imageBufferNew->image[ 0 ];
    unsigned char * const plane1New = &imageBufferNew->image[ planaImageNew.planeSize0 ];
    unsigned char * const plane2New = &imageBufferNew->image[ planaImageNew.planeSize0 + planaImageNew.planeSize1 ];

    const unsigned char * const plane0Old = &imageBufferOld->image[ 0 ];
    const unsigned char * const plane1Old = &imageBufferOld->image[ planaImageOld.planeSize0 ];
    const unsigned char * const plane2Old = &imageBufferOld->image[ planaImageOld.planeSize0 + planaImageOld.planeSize1 ];

    #pragma omp parallel sections
    {
        // copy chroma
        #pragma omp section
        {
            std::memcpy( plane0New, plane0Old, planaImageNew.planeSize0 );
        }

        // create U/Cb Plane
        #pragma omp section
        {
            const int bytesPerPixel = 1;
            const int bytesPerNewLine = planaImageNew.stride1;
            const int bytesPerOldLine = planaImageOld.stride1;

            // main part
            for( int y = 0; y < planaImageNew.height1; ++y )
            {
                const int yOld = y * 2;

                for( int x = 0; x < planaImageNew.width1; ++x )
                {
                    const int xOld = x * 2;

                    const int xOldByteOffset_0_0 = bytesPerPixel *   ( xOld + 0 );
                    const int yOldByteOffset_0_0 = bytesPerOldLine * ( yOld + 0 );
                    const int xOldByteOffset_0_1 = bytesPerPixel *   ( xOld + 0 );
                    const int yOldByteOffset_0_1 = bytesPerOldLine * ( yOld + 1 );
                    const int xOldByteOffset_1_0 = bytesPerPixel *   ( xOld + 1 );
                    const int yOldByteOffset_1_0 = bytesPerOldLine * ( yOld + 0 );
                    const int xOldByteOffset_1_1 = bytesPerPixel *   ( xOld + 1 );
                    const int yOldByteOffset_1_1 = bytesPerOldLine * ( yOld + 1 );

                    int sum = 0;
                    sum += plane1Old[ xOldByteOffset_0_0 + yOldByteOffset_0_0 ];
                    sum += plane1Old[ xOldByteOffset_1_0 + yOldByteOffset_1_0 ];
                    sum += plane1Old[ xOldByteOffset_0_1 + yOldByteOffset_0_1 ];
                    sum += plane1Old[ xOldByteOffset_1_1 + yOldByteOffset_1_1 ];

                    const int xNewByteOffset = bytesPerPixel * x;
                    const int yNewByteOffset = bytesPerNewLine * y;

                    plane1New[ xNewByteOffset + yNewByteOffset ] = sum / 4;
                }
            }

            // remaining part at the right side
            if( ( planaImageNew.width1 * 2 ) > planaImageOld.width1 )
            {
                const int xOld = ( planaImageOld.width1 - 1 );

                for( int y = 0; y < planaImageNew.height1; ++y )
                {
                    const int yOld = y * 2;

                    const int xOldByteOffset_0_0 = bytesPerPixel *   ( xOld + 0 );
                    const int yOldByteOffset_0_0 = bytesPerOldLine * ( yOld + 0 );
                    const int xOldByteOffset_0_1 = bytesPerPixel *   ( xOld + 0 );
                    const int yOldByteOffset_0_1 = bytesPerOldLine * ( yOld + 1 );

                    int sum = 0;
                    sum += plane1Old[ xOldByteOffset_0_0 + yOldByteOffset_0_0 ];
                    sum += plane1Old[ xOldByteOffset_0_1 + yOldByteOffset_0_1 ];
                    
                    const int xNewByteOffset = bytesPerPixel * ( planaImageNew.width1 - 1 );
                    const int yNewByteOffset = bytesPerNewLine * y;

                    plane1New[ xNewByteOffset + yNewByteOffset ] = sum / 2;
                }
            }

            // remaining part at the bottom
            if( ( planaImageNew.height1 * 2 ) > planaImageOld.height1 )
            {
                const int yOld = ( planaImageOld.height1 - 1 );

                for( int x = 0; x < planaImageNew.width1; ++x )
                {
                    const int xOld = x * 2;

                    const int xOldByteOffset_0_0 = bytesPerPixel *   ( xOld + 0 );
                    const int yOldByteOffset_0_0 = bytesPerOldLine * ( yOld + 0 );
                    const int xOldByteOffset_1_0 = bytesPerPixel *   ( xOld + 1 );
                    const int yOldByteOffset_1_0 = bytesPerOldLine * ( yOld + 0 );

                    int sum = 0;
                    sum += plane1Old[ xOldByteOffset_0_0 + yOldByteOffset_0_0 ];
                    sum += plane1Old[ xOldByteOffset_1_0 + yOldByteOffset_1_0 ];

                    const int xNewByteOffset = bytesPerPixel * x;
                    const int yNewByteOffset = bytesPerNewLine * ( planaImageNew.height1 - 1 );

                    plane1New[ xNewByteOffset + yNewByteOffset ] = sum / 2;
                }
            }

            // remaining pixel at the bottom right
            if(    ( ( planaImageNew.width1 * 2 ) > planaImageOld.width1 )
                && ( ( planaImageNew.height1 * 2 ) > planaImageOld.height1 )
              )
            {
                const int xOld = ( planaImageOld.width1 - 1 );
                const int yOld = ( planaImageOld.height1 - 1 );

                const int xOldByteOffset_0_0 = bytesPerPixel *   ( xOld + 0 );
                const int yOldByteOffset_0_0 = bytesPerOldLine * ( yOld + 0 );

                int sum = 0;
                sum += plane1Old[ xOldByteOffset_0_0 + yOldByteOffset_0_0 ];

                const int xNewByteOffset = bytesPerPixel * ( planaImageNew.width1 - 1 );
                const int yNewByteOffset = bytesPerNewLine * ( planaImageNew.height1 - 1 ); 

                plane1New[ xNewByteOffset + yNewByteOffset ] = sum;
            }
        }

        // create V/Cr Plane
        #pragma omp section
        {
            const int bytesPerPixel = 1;
            const int bytesPerNewLine = planaImageNew.stride2;
            const int bytesPerOldLine = planaImageOld.stride2;

            // main part
            for( int y = 0; y < planaImageNew.height2; ++y )
            {
                const int yOld = y * 2;

                for( int x = 0; x < planaImageNew.width2; ++x )
                {
                    const int xOld = x * 2;

                    const int xOldByteOffset_0_0 = bytesPerPixel *   ( xOld + 0 );
                    const int yOldByteOffset_0_0 = bytesPerOldLine * ( yOld + 0 );
                    const int xOldByteOffset_0_1 = bytesPerPixel *   ( xOld + 0 );
                    const int yOldByteOffset_0_1 = bytesPerOldLine * ( yOld + 1 );
                    const int xOldByteOffset_1_0 = bytesPerPixel *   ( xOld + 1 );
                    const int yOldByteOffset_1_0 = bytesPerOldLine * ( yOld + 0 );
                    const int xOldByteOffset_1_1 = bytesPerPixel *   ( xOld + 1 );
                    const int yOldByteOffset_1_1 = bytesPerOldLine * ( yOld + 1 );

                    int sum = 0;
                    sum += plane2Old[ xOldByteOffset_0_0 + yOldByteOffset_0_0 ];
                    sum += plane2Old[ xOldByteOffset_1_0 + yOldByteOffset_1_0 ];
                    sum += plane2Old[ xOldByteOffset_0_1 + yOldByteOffset_0_1 ];
                    sum += plane2Old[ xOldByteOffset_1_1 + yOldByteOffset_1_1 ];

                    const int xNewByteOffset = bytesPerPixel * x;
                    const int yNewByteOffset = bytesPerNewLine * y;

                    plane2New[ xNewByteOffset + yNewByteOffset ] = sum / 4;
                }
            }

            // remaining part at the right side
            if( ( planaImageNew.width2 * 2 ) > planaImageOld.width2 )
            {
                const int xOld = ( planaImageOld.width2 - 1 );

                for( int y = 0; y < planaImageNew.height2; ++y )
                {
                    const int yOld = y * 2;

                    const int xOldByteOffset_0_0 = bytesPerPixel *   ( xOld + 0 );
                    const int yOldByteOffset_0_0 = bytesPerOldLine * ( yOld + 0 );
                    const int xOldByteOffset_0_1 = bytesPerPixel *   ( xOld + 0 );
                    const int yOldByteOffset_0_1 = bytesPerOldLine * ( yOld + 1 );

                    int sum = 0;
                    sum += plane2Old[ xOldByteOffset_0_0 + yOldByteOffset_0_0 ];
                    sum += plane2Old[ xOldByteOffset_0_1 + yOldByteOffset_0_1 ];
                    
                    const int xNewByteOffset = bytesPerPixel * ( planaImageNew.width1 - 1 );
                    const int yNewByteOffset = bytesPerNewLine * y;

                    plane2New[ xNewByteOffset + yNewByteOffset ] = sum / 2;
                }
            }

            // remaining part at the bottom
            if( ( planaImageNew.height2 * 2 ) > planaImageOld.height2 )
            {
                const int yOld = ( planaImageOld.height2 - 1 );

                for( int x = 0; x < planaImageNew.width2; ++x )
                {
                    const int xOld = x * 2;

                    const int xOldByteOffset_0_0 = bytesPerPixel *   ( xOld + 0 );
                    const int yOldByteOffset_0_0 = bytesPerOldLine * ( yOld + 0 );
                    const int xOldByteOffset_1_0 = bytesPerPixel *   ( xOld + 1 );
                    const int yOldByteOffset_1_0 = bytesPerOldLine * ( yOld + 0 );

                    int sum = 0;
                    sum += plane2Old[ xOldByteOffset_0_0 + yOldByteOffset_0_0 ];
                    sum += plane2Old[ xOldByteOffset_1_0 + yOldByteOffset_1_0 ];

                    const int xNewByteOffset = bytesPerPixel * x;
                    const int yNewByteOffset = bytesPerNewLine * ( planaImageNew.height2 - 1 );

                    plane2New[ xNewByteOffset + yNewByteOffset ] = sum / 2;
                }
            }

            // remaining pixel at the bottom right
            if(    ( ( planaImageNew.width2 * 2 ) > planaImageOld.width2 )
                && ( ( planaImageNew.height2 * 2 ) > planaImageOld.height2 )
              )
            {
                const int xOld = ( planaImageOld.width2 - 1 );
                const int yOld = ( planaImageOld.height2 - 1 );

                const int xOldByteOffset_0_0 = bytesPerPixel *   ( xOld + 0 );
                const int yOldByteOffset_0_0 = bytesPerOldLine * ( yOld + 0 );

                int sum = 0;
                sum += plane2Old[ xOldByteOffset_0_0 + yOldByteOffset_0_0 ];

                const int xNewByteOffset = bytesPerPixel * ( planaImageNew.width2 - 1 );
                const int yNewByteOffset = bytesPerNewLine * ( planaImageNew.height2 - 1 ); 

                plane2New[ xNewByteOffset + yNewByteOffset ] = sum;
            }
        }
    }

    // collect information
    ret.m_pixelFormat = image.getPixelFormat();
    ret.m_colorspace = image.getColorspace();
    ret.m_bitsPerPixelAndChannel = image.getBitsPerPixelAndChannel();
    ret.m_chrominanceSubsampling = ChrominanceSubsampling::CS_420;
    ret.m_imageBuffer = imageBufferNew;
    ret.m_width = width;
    ret.m_height = height;

    return ret;
}

ImageJfif ImageJfif::convertChrominanceSubsampling_420to444( const ImageJfif & image )
{
    ImageJfif ret;

    // check image
    if( !image.m_imageBuffer )
    {
        LOG4CXX_ERROR( loggerImage, "imageBuffer is a nullptr" );
        return ret;
    }

    // preparation
    const int width      = image.getWidth();
    const int height     = image.getHeight();

    PlanarImageDesc planaImageNew = calcPlanaerImageDescForYUV( width, height, ChrominanceSubsampling::CS_444, TJ_PAD );
    PlanarImageDesc planaImageOld = calcPlanaerImageDescForYUV( width, height, ChrominanceSubsampling::CS_420, TJ_PAD );

    ImageBufferShrdPtr imageBufferNew = std::make_shared<ImageBuffer>( planaImageNew.bufferSize );
    ImageBufferShrdPtr imageBufferOld = image.getImageBuffer();

    // if( ( planaImageNew.planeSize0 + planaImageNew.planeSize1 + planaImageNew.planeSize2 ) != planaImageNew.bufferSize )
    // {
    //     LOG4CXX_ERROR( loggerImage, "buffer size mismatch (" << __FILE__ << ", " << __LINE__ << ")" );
    //     return ret;
    // }

    // if( planeSize0New != planeSize0Old )
    // {
    //     LOG4CXX_ERROR( loggerImage, 
    //         "buffersize mismatch; " 
    //         << "planeSize0New ("
    //         << planeSize0New
    //         << ") != planeSize0Old ("
    //         << planeSize0Old
    //         << ")"
    //     );
    //     return ret;
    // }    

    unsigned char * const plane0New = &imageBufferNew->image[ 0 ];
    unsigned char * const plane1New = &imageBufferNew->image[ planaImageNew.planeSize0 ];
    unsigned char * const plane2New = &imageBufferNew->image[ planaImageNew.planeSize0 + planaImageNew.planeSize1 ];

    const unsigned char * const plane0Old = &imageBufferOld->image[ 0 ];
    const unsigned char * const plane1Old = &imageBufferOld->image[ planaImageOld.planeSize0 ];
    const unsigned char * const plane2Old = &imageBufferOld->image[ planaImageOld.planeSize0 + planaImageOld.planeSize1 ];

    #pragma omp parallel sections
    {
        // copy chroma
        #pragma omp section
        {
            std::memcpy( plane0New, plane0Old, planaImageNew.planeSize0 );
        }

        // create U/Cb Plane
        #pragma omp section
        {
            const int bytesPerPixel = 1;
            const int bytesPerNewLine = planaImageNew.stride1;
            const int bytesPerOldLine = planaImageOld.stride1;

            // main part
            for( int y = 0; y < ( planaImageNew.height1 / 2 ); ++y )
            {
                for( int x = 0; x < ( planaImageNew.width1 / 2 ); ++x )
                {
                    const int xOldByteOffset = bytesPerPixel * x;
                    const int yOldByteOffset = bytesPerOldLine * y;

                    const unsigned char value = plane1Old[ xOldByteOffset + yOldByteOffset ];

                    const int xNewByteOffset_0_0 = bytesPerPixel   * ( x * 2 + 0 );
                    const int yNewByteOffset_0_0 = bytesPerNewLine * ( y * 2 + 0 );
                    const int xNewByteOffset_0_1 = bytesPerPixel   * ( x * 2 + 0 );
                    const int yNewByteOffset_0_1 = bytesPerNewLine * ( y * 2 + 1 );
                    const int xNewByteOffset_1_0 = bytesPerPixel   * ( x * 2 + 1 );
                    const int yNewByteOffset_1_0 = bytesPerNewLine * ( y * 2 + 0 );
                    const int xNewByteOffset_1_1 = bytesPerPixel   * ( x * 2 + 1 );
                    const int yNewByteOffset_1_1 = bytesPerNewLine * ( y * 2 + 1 );

                    plane1New[ xNewByteOffset_0_0 + yNewByteOffset_0_0 ] = value;
                    plane1New[ xNewByteOffset_1_0 + yNewByteOffset_1_0 ] = value;
                    plane1New[ xNewByteOffset_0_1 + yNewByteOffset_0_1 ] = value;
                    plane1New[ xNewByteOffset_1_1 + yNewByteOffset_1_1 ] = value;
                }
            }

            // remaining part at the right side
            if( ( planaImageNew.width1 / 2 ) < planaImageOld.width1 )
            {
                const int x = planaImageOld.width1 - 1;

                for( int y = 0; y < ( planaImageNew.height1 / 2 ); ++y )
                {
                    const int xOldByteOffset = bytesPerPixel * x;
                    const int yOldByteOffset = bytesPerOldLine * y;

                    const unsigned char value = plane1Old[ xOldByteOffset + yOldByteOffset ];

                    const int xNewByteOffset_0_0 = bytesPerPixel   * ( x * 2 + 0 );
                    const int yNewByteOffset_0_0 = bytesPerNewLine * ( y * 2 + 0 );
                    const int xNewByteOffset_0_1 = bytesPerPixel   * ( x * 2 + 0 );
                    const int yNewByteOffset_0_1 = bytesPerNewLine * ( y * 2 + 1 );

                    plane1New[ xNewByteOffset_0_0 + yNewByteOffset_0_0 ] = value;
                    plane1New[ xNewByteOffset_0_1 + yNewByteOffset_0_1 ] = value;
                }
            }

            // remaining part at the bottom
            if( ( planaImageNew.height1 / 2 ) < planaImageOld.height1 )
            {
                const int y = planaImageOld.height1 - 1;

                for( int x = 0; x < ( planaImageNew.width1 / 2 ); ++x )
                {
                    const int xOldByteOffset = bytesPerPixel * x;
                    const int yOldByteOffset = bytesPerOldLine * y;

                    const unsigned char value = plane1Old[ xOldByteOffset + yOldByteOffset ];

                    const int xNewByteOffset_0_0 = bytesPerPixel   * ( x * 2 + 0 );
                    const int yNewByteOffset_0_0 = bytesPerNewLine * ( y * 2 + 0 );
                    const int xNewByteOffset_1_0 = bytesPerPixel   * ( x * 2 + 1 );
                    const int yNewByteOffset_1_0 = bytesPerNewLine * ( y * 2 + 0 );
                    
                    plane1New[ xNewByteOffset_0_0 + yNewByteOffset_0_0 ] = value;
                    plane1New[ xNewByteOffset_1_0 + yNewByteOffset_1_0 ] = value;
                }
            }

            // remaining pixel at the bottom right
            if(    ( ( planaImageNew.width1 / 2 ) < planaImageOld.width1 )
                && ( ( planaImageNew.height1 / 2 ) < planaImageOld.height1 )
              )
            {
                const int x = planaImageOld.width1 - 1;
                const int y = planaImageOld.height1 - 1;

                const int xOldByteOffset = bytesPerPixel * x;
                const int yOldByteOffset = bytesPerOldLine * y;

                const unsigned char value = plane1Old[ xOldByteOffset + yOldByteOffset ];

                const int xNewByteOffset_0_0 = bytesPerPixel   * ( x * 2 + 0 );
                const int yNewByteOffset_0_0 = bytesPerNewLine * ( y * 2 + 0 );
                
                plane1New[ xNewByteOffset_0_0 + yNewByteOffset_0_0 ] = value;
            }
        }

        // create V/Cr Plane
        #pragma omp section
        {
            const int bytesPerPixel = 1;
            const int bytesPerNewLine = planaImageNew.stride2;
            const int bytesPerOldLine = planaImageOld.stride2;

            // main part
            for( int y = 0; y < ( planaImageNew.height2 / 2 ); ++y )
            {
                for( int x = 0; x < ( planaImageNew.width2 / 2 ); ++x )
                {
                    const int xOldByteOffset = bytesPerPixel * x;
                    const int yOldByteOffset = bytesPerOldLine * y;

                    const unsigned char value = plane2Old[ xOldByteOffset + yOldByteOffset ];

                    const int xNewByteOffset_0_0 = bytesPerPixel   * ( x * 2 + 0 );
                    const int yNewByteOffset_0_0 = bytesPerNewLine * ( y * 2 + 0 );
                    const int xNewByteOffset_0_1 = bytesPerPixel   * ( x * 2 + 0 );
                    const int yNewByteOffset_0_1 = bytesPerNewLine * ( y * 2 + 1 );
                    const int xNewByteOffset_1_0 = bytesPerPixel   * ( x * 2 + 1 );
                    const int yNewByteOffset_1_0 = bytesPerNewLine * ( y * 2 + 0 );
                    const int xNewByteOffset_1_1 = bytesPerPixel   * ( x * 2 + 1 );
                    const int yNewByteOffset_1_1 = bytesPerNewLine * ( y * 2 + 1 );

                    plane2New[ xNewByteOffset_0_0 + yNewByteOffset_0_0 ] = value;
                    plane2New[ xNewByteOffset_1_0 + yNewByteOffset_1_0 ] = value;
                    plane2New[ xNewByteOffset_0_1 + yNewByteOffset_0_1 ] = value;
                    plane2New[ xNewByteOffset_1_1 + yNewByteOffset_1_1 ] = value;
                }
            }

            // remaining part at the right side
            if( ( planaImageNew.width2 / 2 ) < planaImageOld.width2 )
            {
                const int x = planaImageOld.width2 - 1;

                for( int y = 0; y < ( planaImageNew.height2 / 2 ); ++y )
                {
                    const int xOldByteOffset = bytesPerPixel * x;
                    const int yOldByteOffset = bytesPerOldLine * y;

                    const unsigned char value = plane2Old[ xOldByteOffset + yOldByteOffset ];

                    const int xNewByteOffset_0_0 = bytesPerPixel   * ( x * 2 + 0 );
                    const int yNewByteOffset_0_0 = bytesPerNewLine * ( y * 2 + 0 );
                    const int xNewByteOffset_0_1 = bytesPerPixel   * ( x * 2 + 0 );
                    const int yNewByteOffset_0_1 = bytesPerNewLine * ( y * 2 + 1 );

                    plane2New[ xNewByteOffset_0_0 + yNewByteOffset_0_0 ] = value;
                    plane2New[ xNewByteOffset_0_1 + yNewByteOffset_0_1 ] = value;
                }
            }

            // remaining part at the bottom
            if( ( planaImageNew.height2 / 2 ) < planaImageOld.height2 )
            {
                const int y = planaImageOld.height2 - 1;

                for( int x = 0; x < ( planaImageNew.width2 / 2 ); ++x )
                {
                    const int xOldByteOffset = bytesPerPixel * x;
                    const int yOldByteOffset = bytesPerOldLine * y;

                    const unsigned char value = plane2Old[ xOldByteOffset + yOldByteOffset ];

                    const int xNewByteOffset_0_0 = bytesPerPixel   * ( x * 2 + 0 );
                    const int yNewByteOffset_0_0 = bytesPerNewLine * ( y * 2 + 0 );
                    const int xNewByteOffset_1_0 = bytesPerPixel   * ( x * 2 + 1 );
                    const int yNewByteOffset_1_0 = bytesPerNewLine * ( y * 2 + 0 );
                    
                    plane2New[ xNewByteOffset_0_0 + yNewByteOffset_0_0 ] = value;
                    plane2New[ xNewByteOffset_1_0 + yNewByteOffset_1_0 ] = value;
                }
            }

            // remaining pixel at the bottom right
            if(    ( ( planaImageNew.width2 / 2 ) < planaImageOld.width2 )
                && ( ( planaImageNew.height2 / 2 ) < planaImageOld.height2 )
              )
            {
                const int x = planaImageOld.width2 - 1;
                const int y = planaImageOld.height2 - 1;

                const int xOldByteOffset = bytesPerPixel * x;
                const int yOldByteOffset = bytesPerOldLine * y;

                const unsigned char value = plane2Old[ xOldByteOffset + yOldByteOffset ];

                const int xNewByteOffset_0_0 = bytesPerPixel   * ( x * 2 + 0 );
                const int yNewByteOffset_0_0 = bytesPerNewLine * ( y * 2 + 0 );
                
                plane2New[ xNewByteOffset_0_0 + yNewByteOffset_0_0 ] = value;
            }
        }
    }

    // collect information
    ret.m_pixelFormat = image.getPixelFormat();
    ret.m_colorspace = image.getColorspace();
    ret.m_bitsPerPixelAndChannel = image.getBitsPerPixelAndChannel();
    ret.m_chrominanceSubsampling = ChrominanceSubsampling::CS_444;
    ret.m_imageBuffer = imageBufferNew;
    ret.m_width = width;
    ret.m_height = height;

    return ret;
}

ImageJfif ImageJfif::getImageWithChrominanceSubsampling( ChrominanceSubsampling::VALUE cs )
{
    return convertChrominanceSubsampling( *this, cs );
}

ImageJfif ImageJfif::getCompressedDecompressedImage( int quality, ChrominanceSubsampling::VALUE cs )
{
    ImageBufferShrdPtr compressedImage   = compress( *this, quality, cs );
    ImageJfif          decompressedImage = decompress( compressedImage );
    return decompressedImage;
}

ChrominanceSubsampling::VALUE ImageJfif::convertTjJpegSubsamp( int value )
{
    switch( value )
    {
        case TJSAMP_444:  return ChrominanceSubsampling::CS_444;
        case TJSAMP_422:  return ChrominanceSubsampling::CS_422;
        case TJSAMP_420:  return ChrominanceSubsampling::CS_420;
        case TJSAMP_GRAY: return ChrominanceSubsampling::Gray;
        case TJSAMP_440:  return ChrominanceSubsampling::CS_440;
        case TJSAMP_411:  return ChrominanceSubsampling::CS_411;
        default:          return ChrominanceSubsampling::UNKNOWN;
    }
}

Colorspace::VALUE ImageJfif::convertTjJpegColorspace( int value )
{
    switch( value )
    {
        case TJCS_RGB:   return Colorspace::RGB;
        case TJCS_YCbCr: return Colorspace::YCbCr;
        case TJCS_GRAY:  return Colorspace::Gray;
        case TJCS_CMYK:  return Colorspace::CMYK;
        case TJCS_YCCK:  return Colorspace::YCCK;
        default:         return Colorspace::UNKNOWN;
    }
}

PixelFormat::VALUE ImageJfif::convertTjPixelFormat( int value )
{
    switch( value )
    {
        case TJPF_RGB:   return PixelFormat::RGB;
        case TJPF_BGR:   return PixelFormat::BGR;
        // case TJPF_YCbCr: return PixelFormat::YCbCr_Planar;
        case TJPF_RGBX:  return PixelFormat::RGBX;
        case TJPF_BGRX:  return PixelFormat::BGRX;
        case TJPF_XBGR:  return PixelFormat::XBGR;
        case TJPF_XRGB:  return PixelFormat::XRGB;
        case TJPF_GRAY:  return PixelFormat::GRAY;
        case TJPF_RGBA:  return PixelFormat::RGBA;
        case TJPF_BGRA:  return PixelFormat::BGRA;
        case TJPF_ABGR:  return PixelFormat::ABGR;
        case TJPF_ARGB:  return PixelFormat::ARGB;
        case TJPF_CMYK:  return PixelFormat::CMYK;
        default:      return PixelFormat::UNKNOWN;
    }
}

} //namespace imageshrink
