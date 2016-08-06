
#ifndef IMAGEVARIANCE_H_
#define IMAGEVARIANCE_H_

// include system headers
#include <memory> // for smart pointer

// include application headers
#include "ImageInterface.h"

namespace imageshrink
{

// create convenient types
class ImageVariance;
typedef std::shared_ptr<ImageVariance> ImageVarianceShrdPtr;
typedef std::weak_ptr<ImageVariance>   ImageVarianceWkPtr;

// declaration
class ImageVariance
: public ImageInterface
{
    //********** PRELIMINARY **********
    public:

    //********** (DE/CON)STRUCTORS **********
    public:
        ImageVariance();
        ImageVariance( const ImageInterface & image, const ImageInterface & averageImage );
        virtual ~ImageVariance() {}

    protected:

    private:

    //********** ATTRIBUTES **********
    public:

    protected:

    private:
        PixelFormat::VALUE            m_pixelFormat;
        Colorspace::VALUE             m_colorspace;
        BitsPerPixelAndChannel::VALUE m_bitsPerPixelAndChannel;

        ImageBufferShrdPtr            m_imageBuffer;
        int                           m_width;
        int                           m_height;

    //********** METHODS **********
    public:
        // implement ImageInterface
        virtual PixelFormat::VALUE getPixelFormat() const { return m_pixelFormat; }
        virtual Colorspace::VALUE getColorspace() const { return m_colorspace; }
        virtual BitsPerPixelAndChannel::VALUE getBitsPerPixelAndChannel() const { return m_bitsPerPixelAndChannel; }
        virtual ImageBufferShrdPtr getImageBuffer() const { return m_imageBuffer; }
        virtual int getWidth() const { return m_width; }
        virtual int getHeight() const { return m_height; }
        virtual bool isImageValid() const { return static_cast<bool>(m_imageBuffer); }
        virtual void reset();

        // own functions
        // ...

    protected:

    private:

        ImageVariance calcVarianceImage( const ImageInterface & image, const ImageInterface & averageImage );

}; //class

} //namespace imageshrink

#endif //IMAGEVARIANCE_H_
