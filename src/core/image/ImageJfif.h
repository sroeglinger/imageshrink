
#ifndef IMAGEJFIF_H_
#define IMAGEJFIF_H_

// include system headers
#include <memory> // for smart pointer

// include application headers
#include "ImageInterface.h"
#include "stringShrdPtr.h"
#include "enumChrominanceSubsampling.h"

namespace imageshrink
{

// create convenient types
class ImageJfif;
typedef std::shared_ptr<ImageJfif> ImageJfifShrdPtr;
typedef std::weak_ptr<ImageJfif>   ImageJfifWkPtr;

// declaration
class ImageJfif
: public ImageInterface
{
    //********** PRELIMINARY **********
    public:

    //********** (DE/CON)STRUCTORS **********
    public:
        ImageJfif();
        ImageJfif( stringConstShrdPtr path );
        ImageJfif( const std::string & path );
        virtual ~ImageJfif() {}

    protected:

    private:

    //********** ATTRIBUTES **********
    public:

    protected:

    private:
        PixelFormat::VALUE            m_pixelFormat;
        Colorspace::VALUE             m_colorspace;
        BitsPerPixelAndChannel::VALUE m_pitsPerPixelAndChannel;

        ImageBufferShrdPtr            m_imageBuffer;
        int                           m_width;
        int                           m_height;

    //********** METHODS **********
    public:
        // implement ImageInterface
        virtual PixelFormat::VALUE getPixelFormat() const { return m_pixelFormat; }
        virtual Colorspace::VALUE getColorspace() const { return m_colorspace; }
        virtual BitsPerPixelAndChannel::VALUE getBitsPerPixelAndChannel() const { return m_pitsPerPixelAndChannel; }
        virtual ImageBufferShrdPtr getImageBuffer() const { return m_imageBuffer; }
        virtual int getWidth() const { return m_width; }
        virtual int getHeight() const { return m_height; }
        virtual bool isImageValid() const { return static_cast<bool>(m_imageBuffer); }
        virtual void reset();

        // own functions
        ImageJfif getCompressedDecompressedImage( int quality );

    protected:

    private:
        void loadImage( const std::string & path );

        ChrominanceSubsampling::VALUE convertTjJpegSubsamp( int value );
        Colorspace::VALUE convertTjJpegColorspace( int value );
        PixelFormat::VALUE convertTjPixelFormat( int value );
        int convert2Tj( ChrominanceSubsampling::VALUE cs );

        ImageJfif decompress( ImageBufferShrdPtr compressedImage );
        ImageBufferShrdPtr compress( const ImageJfif & notCompressed, int quality = 85, ChrominanceSubsampling::VALUE cs = ChrominanceSubsampling::CS_444 );

}; //class

} //namespace imageshrink

#endif //IMAGEJFIF_H_
