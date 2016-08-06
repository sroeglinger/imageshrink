
#ifndef IMAGECOVARIANCE_H_
#define IMAGECOVARIANCE_H_

// include system headers
#include <memory> // for smart pointer

// include application headers
#include "ImageInterface.h"

namespace imageshrink
{

// create convenient types
class ImageCovariance;
typedef std::shared_ptr<ImageCovariance> ImageCovarianceShrdPtr;
typedef std::weak_ptr<ImageCovariance>   ImageCovarianceWkPtr;

// declaration
class ImageCovariance
: public ImageInterface
{
    //********** PRELIMINARY **********
    public:

    //********** (DE/CON)STRUCTORS **********
    public:
        ImageCovariance();
        ImageCovariance( const ImageInterface & image1, const ImageInterface & averageImage1, const ImageInterface & image2, const ImageInterface & averageImage2 );
        virtual ~ImageCovariance() {}

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

        ImageCovariance calcCovarianceImage( const ImageInterface & image1, const ImageInterface & averageImage1, const ImageInterface & image2, const ImageInterface & averageImage2 );

}; //class

} //namespace imageshrink

#endif //IMAGECOVARIANCE_H_
