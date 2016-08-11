
#ifndef IMAGEINTERFACE_H_
#define IMAGEINTERFACE_H_

// include system headers
#include <memory> // for smart pointer

// include application headers
#include "enumPixelFormat.h"
#include "enumColorspace.h"
#include "enumBitsPerPixelAndChannel.h"
#include "enumChrominanceSubsampling.h"
#include "ImageBuffer.h"

namespace imageshrink
{

// create convenient types
class ImageInterface;
typedef std::shared_ptr<ImageInterface> ImageInterfaceShrdPtr;
typedef std::weak_ptr<ImageInterface>   ImageInterfaceWkPtr;

// declaration
class ImageInterface
{
    //********** PRELIMINARY **********
    public:

    //********** (DE/CON)STRUCTORS **********
    public:

    protected:
        ImageInterface() {}
        virtual ~ImageInterface() {}

    private:

    //********** ATTRIBUTES **********
    public:

    protected:

    private:

    //********** METHODS **********
    public:
        virtual PixelFormat::VALUE getPixelFormat() const = 0;
        virtual Colorspace::VALUE getColorspace() const = 0;
        virtual BitsPerPixelAndChannel::VALUE getBitsPerPixelAndChannel() const = 0;
        virtual ChrominanceSubsampling::VALUE getChrominanceSubsampling() const = 0;
        virtual ImageBufferShrdPtr getImageBuffer() const = 0;
        virtual int getWidth() const = 0;
        virtual int getHeight() const = 0;
        virtual bool isImageValid() const = 0;
        virtual void reset() = 0; 

    protected:

    private:

}; //class

} //namespace imageshrink

#endif //IMAGEINTERFACE_H_
