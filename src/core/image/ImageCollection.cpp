
// include own headers
#include "ImageCollection.h"

namespace imageshrink
{

void ImageCollection::addImage( const std::string & name, ImageInterfaceShrdPtr image )
{
    m_stringImageMap[ name ] = image;
}

void ImageCollection::addImage( stringConstShrdPtr name, ImageInterfaceShrdPtr image )
{
    if( name )
    {
        addImage( *name, image );
    }
}

void ImageCollection::removeImage( const std::string & name, ImageInterfaceShrdPtr & image )
{
    auto search = m_stringImageMap.find( name );
    if( search != m_stringImageMap.end() )
    {
        image = search->second;
    }
    else
    {
        image.reset();
    }
}

void ImageCollection::removeImage( stringConstShrdPtr name, ImageInterfaceShrdPtr & image )
{
    if( name )
    {
        removeImage( *name, image );
    }
}

void ImageCollection::removeImage( const std::string & name )
{
    (void) m_stringImageMap.erase( name );
}

void ImageCollection::removeImage( stringConstShrdPtr name )
{
    if( name )
    {
        removeImage( *name );
    }
}

} //namespace imageshrink
