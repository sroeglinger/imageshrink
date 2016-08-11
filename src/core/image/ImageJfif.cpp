
// include system headers
#include <fstream>      // std::ifstream
#include <limits>       // std::numeric_limits<...>::...
#include <cstring>      // std::memcpy

// include own headers
#include "ImageJfif.h"

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

    ImageBufferShrdPtr compressedImage = ImageBufferShrdPtr( new ImageBuffer(jpegSize) );
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
    const int tjOutputPixelFormat = TJPF_RGB;

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
        ret.m_pixelFormat            = convertTjPixelFormat( tjOutputPixelFormat );
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
    const int bytesPerPixel = PixelFormat::channelsPerPixel(ret.m_pixelFormat) * BitsPerPixelAndChannel::bytesPerChannel(ret.m_bitsPerPixelAndChannel);
    const int nofPixels     = width * height;
    const int bufferSize    = bytesPerPixel * nofPixels;
    ImageBufferShrdPtr imageBuffer = ImageBufferShrdPtr( new ImageBuffer( bufferSize ) );

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
        ret = ImageBufferShrdPtr( new ImageBuffer(jpegSize) );
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

    // check image
    if( !image.m_imageBuffer )
    {
        LOG4CXX_ERROR( loggerImage, "imageBuffer is a nullptr" );
        return ret;
    }

    // early return if nothing has to be done
    if( image.getChrominanceSubsampling() == cs )
    {
        return image;
    }

    // change chrominance subsampling
    if( image.getChrominanceSubsampling() == ChrominanceSubsampling::CS_444 )
    {
        switch( cs )
        {
            case ChrominanceSubsampling::CS_420: ret = convertChrominanceSubsampling_444to420( image ); done = true; break;
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
    const int subsampNew = convert2Tj(ChrominanceSubsampling::CS_420);
    const int subsampOld = convert2Tj(ChrominanceSubsampling::CS_444);

    const unsigned long yuvPlanarBufferSize = tjBufSizeYUV2( width, /*pad*/ TJ_PAD, height, subsampNew );
    ImageBufferShrdPtr imageBufferNew = ImageBufferShrdPtr( new ImageBuffer( yuvPlanarBufferSize ) );
    ImageBufferShrdPtr imageBufferOld = image.getImageBuffer();

    const int width0New = tjPlaneWidth( 0 /*0 = Y*/,    width, subsampNew );
    const int width1New = tjPlaneWidth( 1 /*1 = U/Cb*/, width, subsampNew );
    const int width2New = tjPlaneWidth( 2 /*2 = V/Cr*/, width, subsampNew );

    const int width0Old = tjPlaneWidth( 0 /*0 = Y*/,    width, subsampOld );
    const int width1Old = tjPlaneWidth( 1 /*1 = U/Cb*/, width, subsampOld );
    const int width2Old = tjPlaneWidth( 2 /*2 = V/Cr*/, width, subsampOld );

    const int height0New = tjPlaneHeight( 0 /*0 = Y*/,    height, subsampNew );
    const int height1New = tjPlaneHeight( 1 /*1 = U/Cb*/, height, subsampNew );
    const int height2New = tjPlaneHeight( 2 /*2 = V/Cr*/, height, subsampNew );

    const int height0Old = tjPlaneHeight( 0 /*0 = Y*/,    height, subsampOld );
    const int height1Old = tjPlaneHeight( 1 /*1 = U/Cb*/, height, subsampOld );
    const int height2Old = tjPlaneHeight( 2 /*2 = V/Cr*/, height, subsampOld );

    const int stride0New = linePadding( width0New );
    const int stride1New = linePadding( width1New );
    const int stride2New = linePadding( width2New );

    const int stride0Old = linePadding( width0Old );
    const int stride1Old = linePadding( width1Old );
    const int stride2Old = linePadding( width2Old );

    const unsigned long planeSize0New = tjPlaneSizeYUV( 0 /*0 = Y*/,    width, stride0New, height, subsampNew );
    const unsigned long planeSize1New = tjPlaneSizeYUV( 1 /*1 = U/Cb*/, width, stride1New, height, subsampNew );
    const unsigned long planeSize2New = tjPlaneSizeYUV( 2 /*2 = V/Cr*/, width, stride2New, height, subsampNew );

    const unsigned long planeSize0Old = tjPlaneSizeYUV( 0 /*0 = Y*/,    width, stride0Old, height, subsampOld );
    const unsigned long planeSize1Old = tjPlaneSizeYUV( 1 /*1 = U/Cb*/, width, stride1Old, height, subsampOld );
    const unsigned long planeSize2Old = tjPlaneSizeYUV( 2 /*2 = V/Cr*/, width, stride2Old, height, subsampOld );


    if( ( planeSize0New + planeSize1New + planeSize2New ) != yuvPlanarBufferSize )
    {
        LOG4CXX_ERROR( loggerImage, "buffersize mismatch" );
        return ret;
    }

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
    unsigned char * const plane1New = &imageBufferNew->image[ planeSize0New ];
    unsigned char * const plane2New = &imageBufferNew->image[ planeSize0New + planeSize1New ];

    const unsigned char * const plane0Old = &imageBufferOld->image[ 0 ];
    const unsigned char * const plane1Old = &imageBufferOld->image[ planeSize0Old ];
    const unsigned char * const plane2Old = &imageBufferOld->image[ planeSize0Old + planeSize1Old ];

    #pragma omp parallel sections
    {
        // copy chroma
        #pragma omp section
        {
            std::memcpy( plane0New, plane0Old, planeSize0New );
        }

        // create U/Cb Plane
        #pragma omp section
        {
            const int bytesPerPixel = 1;
            const int bytesPerNewLine = stride1New;
            const int bytesPerOldLine = stride1Old;

            for( int y = 0; y < height1New; ++y )
            {
                const int yOld = y * 2;

                for( int x = 0; x < width1New; ++x )
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
        }

        // create V/Cr Plane
        #pragma omp section
        {
            const int bytesPerPixel = 1;
            const int bytesPerNewLine = stride2New;
            const int bytesPerOldLine = stride2Old;

            for( int y = 0; y < height2New; ++y )
            {
                const int yOld = y * 2;

                for( int x = 0; x < width2New; ++x )
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
        }
    }

    // collect information
    ret.m_pixelFormat = image.getPixelFormat();
    ret.m_colorspace = image.getColorspace();
    ret.m_bitsPerPixelAndChannel = image.getBitsPerPixelAndChannel();
    ret.m_chrominanceSubsampling = convertTjJpegSubsamp( subsampNew );
    ret.m_imageBuffer = imageBufferNew;
    ret.m_width = width;
    ret.m_height = height;

    return ret;
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
        // case TJPF_YCbCr: return PixelFormat::YCbCr;
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

int ImageJfif::convert2Tj( ChrominanceSubsampling::VALUE cs )
{
    switch( cs )
    {
        case ChrominanceSubsampling::CS_444: return TJSAMP_444;
        case ChrominanceSubsampling::CS_422: return TJSAMP_422;
        case ChrominanceSubsampling::CS_420: return TJSAMP_420;
        case ChrominanceSubsampling::Gray:   return TJSAMP_GRAY;
        case ChrominanceSubsampling::CS_440: return TJSAMP_440;
        case ChrominanceSubsampling::CS_411: return TJSAMP_411;
        default: 
            LOG4CXX_INFO( loggerImage, "There is no TurboJpeg Value for " << ChrominanceSubsampling::toString( cs ) << "." );
            return TJSAMP_444;
    }    
}

int ImageJfif::linePadding( int width )
{
    const int mod = width % TJ_PAD;
    if( mod == 0 )
    {
        return width;
    }
    else
    {
        return width + ( TJ_PAD - mod );
    }
}

} //namespace imageshrink
