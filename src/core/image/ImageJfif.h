
#ifndef IMAGEJFIF_H_
#define IMAGEJFIF_H_

// include system headers
#include <memory> // for smart pointer
#include <list>

// include application headers
#include "ImageInterface.h"
#include "stringShrdPtr.h"

namespace imageshrink
{

// create convenient types
class ImageJfif;
typedef std::shared_ptr<ImageJfif> ImageJfifShrdPtr;
typedef std::weak_ptr<ImageJfif>   ImageJfifWkPtr;

// declaration
class ImageJfif
: public std::enable_shared_from_this<ImageJfif>
, public ImageInterface
{
    //********** PRELIMINARY **********
    public:
        struct Marker
        {
            std::string marker;
            std::string content;
        };

        typedef std::shared_ptr<Marker>  MarkerShrdPtr;
        typedef std::weak_ptr<Marker>    MarkerWkPtr;
        typedef std::list<MarkerShrdPtr> ListOfMarkerShrdPtr;


    //********** (DE/CON)STRUCTORS **********
    public:
        ImageJfif();
        ImageJfif( stringConstShrdPtr path );
        ImageJfif( const std::string & path );
        ImageJfif( const ImageInterface & image );
        ImageJfif( ImageInterfaceShrdPtr image );
        virtual ~ImageJfif() {}

    protected:

    private:

    //********** ATTRIBUTES **********
    public:

    protected:

    private:
        PixelFormat::VALUE            m_pixelFormat;
        Colorspace::VALUE             m_colorspace;
        BitsPerPixelAndChannel::VALUE m_bitsPerPixelAndChannel;
        ChrominanceSubsampling::VALUE m_chrominanceSubsampling;

        ImageBufferShrdPtr            m_imageBuffer;
        int                           m_width;
        int                           m_height;

        ListOfMarkerShrdPtr           m_listOfMarkers;

    //********** METHODS **********
    public:
        // implement ImageInterface
        virtual PixelFormat::VALUE getPixelFormat() const { return m_pixelFormat; }
        virtual Colorspace::VALUE getColorspace() const { return m_colorspace; }
        virtual BitsPerPixelAndChannel::VALUE getBitsPerPixelAndChannel() const { return m_bitsPerPixelAndChannel; }
        virtual ChrominanceSubsampling::VALUE getChrominanceSubsampling() const { return m_chrominanceSubsampling; }
        virtual ImageBufferShrdPtr getImageBuffer() const { return m_imageBuffer; }
        virtual int getWidth() const { return m_width; }
        virtual int getHeight() const { return m_height; }
        virtual bool isImageValid() const { return static_cast<bool>(m_imageBuffer); }
        virtual void reset();

        // own functions
        ImageJfif getCompressedDecompressedImage( int quality, ChrominanceSubsampling::VALUE cs = ChrominanceSubsampling::CS_444 );
        void storeInFile( const std::string & path, int quality = 85, ChrominanceSubsampling::VALUE value = ChrominanceSubsampling::CS_444 );
        void storeInFile( const std::string & path, const ListOfMarkerShrdPtr & markers, int quality = 85, ChrominanceSubsampling::VALUE value = ChrominanceSubsampling::CS_444 );
        ImageJfif getImageWithChrominanceSubsampling( ChrominanceSubsampling::VALUE cs );
        ListOfMarkerShrdPtr getMarkers() { return m_listOfMarkers; }

    protected:

    private:
        void loadImage( const std::string & path );

        ChrominanceSubsampling::VALUE convertTjJpegSubsamp( int value );
        Colorspace::VALUE convertTjJpegColorspace( int value );
        PixelFormat::VALUE convertTjPixelFormat( int value );

        ImageJfif decompress( ImageBufferShrdPtr compressedImage );
        ImageBufferShrdPtr compress( const ImageJfif & notCompressed, int quality = 85, ChrominanceSubsampling::VALUE cs = ChrominanceSubsampling::CS_444 );

        ImageJfif convertChrominanceSubsampling( const ImageJfif & image, ChrominanceSubsampling::VALUE cs );
        ImageJfif convertChrominanceSubsampling_444to420( const ImageJfif & image );
        ImageJfif convertChrominanceSubsampling_420to444( const ImageJfif & image );

        ListOfMarkerShrdPtr copyMarkers( ImageBufferShrdPtr compressedImage );
        ImageBufferShrdPtr enrichCompressedImageWithMakers( ImageBufferShrdPtr compressedImage, const ListOfMarkerShrdPtr & markers );

}; //class

} //namespace imageshrink

#endif //IMAGEJFIF_H_
