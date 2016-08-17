
#ifndef IMAGEDSSIM_H_
#define IMAGEDSSIM_H_

// include system headers
#include <memory> // for smart pointer

// include application headers
#include "ImageInterface.h"
#include "ImageCollection.h"

namespace imageshrink
{

// create convenient types
class ImageDSSIM;
typedef std::shared_ptr<ImageDSSIM> ImageDSSIMShrdPtr;
typedef std::weak_ptr<ImageDSSIM>   ImageDSSIMWkPtr;

// declaration
class ImageDSSIM
: public ImageInterface
{
    //********** PRELIMINARY **********
    public:

    //********** (DE/CON)STRUCTORS **********
    public:
        ImageDSSIM();
        ImageDSSIM( const ImageCollection & imageCollection1, const ImageCollection & imageCollection2 );
        virtual ~ImageDSSIM() {}

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

        double                        m_dssim;
        double                        m_dssimPeak;
        bool                          m_dssimValid;

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
        double getDssim();
        double getDssimPeak();

    protected:

    private:

        ImageDSSIM calcDSSIMImage_RGB( const ImageCollection & imageCollection1, const ImageCollection & imageCollection2 );
        ImageDSSIM calcDSSIMImage_YUV( const ImageCollection & imageCollection1, const ImageCollection & imageCollection2 );

}; //class

} //namespace imageshrink

#endif //IMAGEDSSIM_H_
