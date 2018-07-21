
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
: public std::enable_shared_from_this<ImageCovariance>
, public ImageInterface
{
    //********** PRELIMINARY **********
    public:

    //********** (DE/CON)STRUCTORS **********
    public:
        ImageCovariance();
        ImageCovariance( ImageInterfaceShrdPtr image1, ImageInterfaceShrdPtr averageImage1, ImageInterfaceShrdPtr image2, ImageInterfaceShrdPtr averageImage2, int averaging );
        ImageCovariance( const ImageInterface & image1, const ImageInterface & averageImage1, const ImageInterface & image2, const ImageInterface & averageImage2, int averaging );
        virtual ~ImageCovariance() {}

    protected:

    private:

    //********** ATTRIBUTES **********
    public:

    protected:

    private:
        int                           m_averaging;
    
        PixelFormat::VALUE            m_pixelFormat;
        Colorspace::VALUE             m_colorspace;
        BitsPerPixelAndChannel::VALUE m_bitsPerPixelAndChannel;
        ChrominanceSubsampling::VALUE m_chrominanceSubsampling;

        ImageBufferShrdPtr            m_imageBuffer;
        int                           m_width;
        int                           m_height;

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
        // ...

    protected:

    private:

        ImageCovariance calcCovarianceImage_RGB( const ImageInterface & image1, const ImageInterface & averageImage1, const ImageInterface & image2, const ImageInterface & averageImage2 );
        ImageCovariance calcCovarianceImage_YUV( const ImageInterface & image1, const ImageInterface & averageImage1, const ImageInterface & image2, const ImageInterface & averageImage2 );

}; //class

} //namespace imageshrink

#endif //IMAGECOVARIANCE_H_
