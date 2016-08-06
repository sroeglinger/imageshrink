
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
, m_imageBuffer()
, m_width( 0 )
, m_height( 0 )
{
    m_pixelFormat            = image.getPixelFormat();
    m_colorspace             = image.getColorspace();
    m_bitsPerPixelAndChannel = image.getBitsPerPixelAndChannel();
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
    m_imageBuffer            = image.m_imageBuffer;
    m_width                  = image.m_width;
    m_height                 = image.m_height;
}

void ImageJfif::storeInFile( const std::string & path )
{    
    if( !m_imageBuffer )
    {
        LOG4CXX_ERROR( loggerImage, "m_imageBuffer is a nullptr" );
        return;
    }

    // compress image
    ImageBufferShrdPtr compressedImage = compress( *this, /*quality*/ 85, ChrominanceSubsampling::CS_444 );

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

    tjRet = tjDecompress2(
        jpegDecompressor, 
        reinterpret_cast<const unsigned char*>( compressedImage->image ), 
        compressedImage->size, 
        imageBuffer->image,
        width, 
        0/*pitch*/, 
        height, 
        tjOutputPixelFormat, 
        TJFLAG_FASTDCT
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

    // check image
    if( !notCompressed.m_imageBuffer )
    {
        LOG4CXX_ERROR( loggerImage, "imageBuffer is a nullptr" );
        return ret;
    }

    // compress jpeg
    tjhandle jpegCompressor = tjInitCompress();

    long unsigned int jpegSize = tjBufSize( notCompressed.m_width, notCompressed.m_height, jpegSubsamp );
    unsigned char* compressedImageBuffer = tjAlloc( jpegSize );

    LOG4CXX_INFO( loggerImage, "compress image ..." );

    tjRet = tjCompress2(
        jpegCompressor,
        reinterpret_cast<const unsigned char*>( notCompressed.m_imageBuffer->image ),
        notCompressed.m_width,
        /*pitch*/ 0,
        notCompressed.m_height,
        TJPF_RGB,
        &compressedImageBuffer,
        &jpegSize,
        jpegSubsamp,
        quality,
        TJFLAG_FASTDCT
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

ImageJfif ImageJfif::getCompressedDecompressedImage( int quality )
{
    ImageBufferShrdPtr compressedImage   = compress( *this, quality );
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

} //namespace imageshrink
