
#ifndef IMAGECOLLECTION_H_
#define IMAGECOLLECTION_H_

// include system headers
#include <memory> // for smart pointer
#include <unordered_map>

// include application headers
#include "ImageInterface.h"
#include "stringShrdPtr.h"


namespace imageshrink
{

// create convenient types
class ImageCollection;
typedef std::shared_ptr<ImageCollection> ImageCollectionShrdPtr;
typedef std::weak_ptr<ImageCollection>   ImageCollectionWkPtr;

// declaration
class ImageCollection
: public std::enable_shared_from_this<ImageCollection>
{
    //********** PRELIMINARY **********
    public:

    private:
        typedef std::unordered_map<std::string, ImageInterfaceShrdPtr> StringImageMap;

    //********** (DE/CON)STRUCTORS **********
    public:
        ImageCollection()
        : m_stringImageMap()
        {}

        virtual ~ImageCollection() {}

    protected:

    private:

    //********** ATTRIBUTES **********
    public:

    protected:

    private:
        StringImageMap   m_stringImageMap;

    //********** METHODS **********
    public:
        void addImage( const std::string & name, ImageInterfaceShrdPtr image );
        void addImage( stringConstShrdPtr name, ImageInterfaceShrdPtr image );

        ImageInterfaceShrdPtr getImage( const std::string & name ) const;
        ImageInterfaceShrdPtr getImage( stringConstShrdPtr name ) const;

        void removeImage( const std::string & name, ImageInterfaceShrdPtr & image );
        void removeImage( stringConstShrdPtr name, ImageInterfaceShrdPtr & image );
        void removeImage( const std::string & name );
        void removeImage( stringConstShrdPtr name );

    protected:

    private:

}; //class

} //namespace imageshrink

#endif //IMAGECOLLECTION_H_
