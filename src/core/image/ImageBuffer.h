
#ifndef IMAGEBUFFER_H_
#define IMAGEBUFFER_H_

// include system headers
#include <memory> // for smart pointer

// include application headers

namespace imageshrink
{

// create convenient types
struct ImageBuffer;
typedef std::shared_ptr<ImageBuffer> ImageBufferShrdPtr;
typedef std::weak_ptr<ImageBuffer>   ImageBufferWkPtr;

// declaration
struct ImageBuffer
{
    ImageBuffer() 
    : image( nullptr )
    , size( 0 ) 
    {
        // nothing
    }

    ImageBuffer( int s )
    : image( nullptr )
    , size( 0 )
    {
        image = new unsigned char[s];
        size  = s;
    }

    ~ImageBuffer()
    {
        delete[] image;
        image = nullptr;
        size = 0;
    }

    unsigned char * image;
    int             size;
};

} //namespace imageshrink

#endif //IMAGEBUFFER_H_
