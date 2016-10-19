
// include system headers
#include <string.h> // for memcpy

// include own headers
#include "ImageJfif.h"

// include application headers

// include 3rd party headers
#ifdef USE_LOG4CXX
#include <log4cxx/logger.h>
#endif //USE_LOG4CXX

namespace imageshrink
{

#ifdef USE_LOG4CXX
static log4cxx::LoggerPtr loggerImage( log4cxx::Logger::getLogger( "image" ) );
#endif //USE_LOG4CXX

int getLengthOfMarker( const char * const & image, int & pos )
{
    const int byte1 = static_cast<unsigned char>( image[ pos + 0 ] );
    const int byte2 = static_cast<unsigned char>( image[ pos + 1 ] );
    
    const int ret = ( byte1 << 8 ) + ( byte2 );
    return ret;
}
    
ImageJfif::ListOfMarkerShrdPtr ImageJfif::copyMarkers( ImageBufferShrdPtr compressedImage )
{
    // preparation
    int pos = 0;
    ListOfMarkerShrdPtr retList;
    
    if( !compressedImage )
    {
#ifdef USE_LOG4CXX
        LOG4CXX_ERROR( loggerImage, "compressedImage is a nullptr" );
#endif //USE_LOG4CXX
        return retList;
    }
    
    if(    ( compressedImage->image == nullptr )
        || ( compressedImage->size == 0 )
      )
    {
#ifdef USE_LOG4CXX
        LOG4CXX_ERROR( loggerImage, "compressedImage->image is a nullptr" );
#endif //USE_LOG4CXX
        return retList;
    }
    
    const char * const image = reinterpret_cast<const char * const>( compressedImage->image );
    const int size           = compressedImage->size;
    
    // check magic number
    if( std::string( &image[pos], 2 ) != "\xff\xd8" )
    {
#ifdef USE_LOG4CXX
        LOG4CXX_ERROR( loggerImage, "incorrect magic number" );
#endif //USE_LOG4CXX
        return retList;
    }
    pos += 2;

    // parse markers
#ifdef USE_LOG4CXX
    LOG4CXX_INFO( loggerImage, "parse JFIF file and copy markers ..." );
#endif //USE_LOG4CXX

    bool withinSOS = false;
    bool seenEOI = false;
    while( pos < size )
    {
        if( withinSOS )
        {
            while( !(    ( static_cast<unsigned char>( image[pos+0] ) == 0xff )
                      && ( static_cast<unsigned char>( image[pos+1] ) != 0x0 )
                    )
                 )
            {
                pos++;
            }
            
            withinSOS = false;
        }
        
        const std::string marker( &image[pos], 2 );
        pos += 2;
        
        if( marker == "\xff\xe0" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "APP0 (JFIF) marker at possition 0x" << std::hex << ( pos - 2 ) << " detected" );
#endif //USE_LOG4CXX
            pos += getLengthOfMarker( image, pos );
        }
        else if( marker == "\xff\xe1" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "APP1 (EXIF / XMP) marker at possition 0x" << std::hex << ( pos - 2 ) << " detected" );
#endif //USE_LOG4CXX
            const int length = getLengthOfMarker( image, pos );

            MarkerShrdPtr marker = std::make_shared< Marker >();
            marker->marker = "\xff\xe1";
            marker->content = std::string( &image[pos], length );
            retList.push_back( marker );

            pos += length;
        }
        else if( marker == "\xff\xe2" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "APP2 (ICC) marker at possition 0x" << std::hex << ( pos - 2 ) << " detected" );
#endif //USE_LOG4CXX
            const int length = getLengthOfMarker( image, pos );

            MarkerShrdPtr marker = std::make_shared< Marker >();
            marker->marker = "\xff\xe2";
            marker->content = std::string( &image[pos], length );
            retList.push_back( marker );

            pos += length;
        }
        else if( marker == "\xff\xe3" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "APP3 (?) marker at possition 0x" << std::hex << ( pos - 2 ) << " detected" );
#endif //USE_LOG4CXX
            const int length = getLengthOfMarker( image, pos );

            MarkerShrdPtr marker = std::make_shared< Marker >();
            marker->marker = "\xff\xe3";
            marker->content = std::string( &image[pos], length );
            retList.push_back( marker );

            pos += length;
        }
        else if( marker == "\xff\xe4" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "APP4 (?) marker at possition 0x" << std::hex << ( pos - 2 ) << " detected" );
#endif //USE_LOG4CXX
            const int length = getLengthOfMarker( image, pos );

            MarkerShrdPtr marker = std::make_shared< Marker >();
            marker->marker = "\xff\xe4";
            marker->content = std::string( &image[pos], length );
            retList.push_back( marker );

            pos += length;
        }
        else if( marker == "\xff\xe5" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "APP5 (?) marker at possition 0x" << std::hex << ( pos - 2 ) << " detected" );
#endif //USE_LOG4CXX
            const int length = getLengthOfMarker( image, pos );

            MarkerShrdPtr marker = std::make_shared< Marker >();
            marker->marker = "\xff\xe5";
            marker->content = std::string( &image[pos], length );
            retList.push_back( marker );

            pos += length;
        }
        else if( marker == "\xff\xe6" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "APP6 (?) marker at possition 0x" << std::hex << ( pos - 2 ) << " detected" );
#endif //USE_LOG4CXX
            const int length = getLengthOfMarker( image, pos );

            MarkerShrdPtr marker = std::make_shared< Marker >();
            marker->marker = "\xff\xe6";
            marker->content = std::string( &image[pos], length );
            retList.push_back( marker );

            pos += length;
        }
        else if( marker == "\xff\xe7" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "APP7 (?) marker at possition 0x" << std::hex << ( pos - 2 ) << " detected" );
#endif //USE_LOG4CXX
            const int length = getLengthOfMarker( image, pos );

            MarkerShrdPtr marker = std::make_shared< Marker >();
            marker->marker = "\xff\xe7";
            marker->content = std::string( &image[pos], length );
            retList.push_back( marker );

            pos += length;
        }
        else if( marker == "\xff\xe8" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "APP8 (?) marker at possition 0x" << std::hex << ( pos - 2 ) << " detected" );
#endif //USE_LOG4CXX
            const int length = getLengthOfMarker( image, pos );

            MarkerShrdPtr marker = std::make_shared< Marker >();
            marker->marker = "\xff\xe8";
            marker->content = std::string( &image[pos], length );
            retList.push_back( marker );

            pos += length;
        }
        else if( marker == "\xff\xe9" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "APP9 (?) marker at possition 0x" << std::hex << ( pos - 2 ) << " detected" );
#endif //USE_LOG4CXX
            const int length = getLengthOfMarker( image, pos );

            MarkerShrdPtr marker = std::make_shared< Marker >();
            marker->marker = "\xff\xe9";
            marker->content = std::string( &image[pos], length );
            retList.push_back( marker );

            pos += length;
        }
        else if( marker == "\xff\xea" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "APP10 (?) marker at possition 0x" << std::hex << ( pos - 2 ) << " detected" );
#endif //USE_LOG4CXX
            const int length = getLengthOfMarker( image, pos );

            MarkerShrdPtr marker = std::make_shared< Marker >();
            marker->marker = "\xff\xea";
            marker->content = std::string( &image[pos], length );
            retList.push_back( marker );

            pos += length;
        }
        else if( marker == "\xff\xeb" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "APP11 (?) marker at possition 0x" << std::hex << ( pos - 2 ) << " detected" );
#endif //USE_LOG4CXX
            const int length = getLengthOfMarker( image, pos );

            MarkerShrdPtr marker = std::make_shared< Marker >();
            marker->marker = "\xff\xeb";
            marker->content = std::string( &image[pos], length );
            retList.push_back( marker );

            pos += length;
        }
        else if( marker == "\xff\xec" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "APP12 (?) marker at possition 0x" << std::hex << ( pos - 2 ) << " detected" );
#endif //USE_LOG4CXX
            const int length = getLengthOfMarker( image, pos );

            MarkerShrdPtr marker = std::make_shared< Marker >();
            marker->marker = "\xff\xec";
            marker->content = std::string( &image[pos], length );
            retList.push_back( marker );

            pos += length;
        }
        else if( marker == "\xff\xed" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "APP13 (IPTC) marker at possition 0x" << std::hex << ( pos - 2 ) << " detected" );
#endif //USE_LOG4CXX
            const int length = getLengthOfMarker( image, pos );

            MarkerShrdPtr marker = std::make_shared< Marker >();
            marker->marker = "\xff\xed";
            marker->content = std::string( &image[pos], length );
            retList.push_back( marker );

            pos += length;
        }
        else if( marker == "\xff\xee" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "APP14 (?) marker at possition 0x" << std::hex << ( pos - 2 ) << " detected" );
#endif //USE_LOG4CXX
            const int length = getLengthOfMarker( image, pos );

            MarkerShrdPtr marker = std::make_shared< Marker >();
            marker->marker = "\xff\xee";
            marker->content = std::string( &image[pos], length );
            retList.push_back( marker );

            pos += length;
        }
        else if( marker == "\xff\xdb" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "DQT marker at possition 0x" << std::hex << ( pos - 2 ) << " detected (Define Quantization Table)" );
#endif //USE_LOG4CXX
            pos += getLengthOfMarker( image, pos );
        }
        else if( marker == "\xff\xc0" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "SOF0 marker at possition 0x" << std::hex << ( pos - 2 ) << " detected (Start Of Frame (baseline DCT))" );
#endif //USE_LOG4CXX
            pos += getLengthOfMarker( image, pos );
        }
        else if( marker == "\xff\xc2" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "SOF2 marker at possition 0x" << std::hex << ( pos - 2 ) << " detected (Start Of Frame (progressive DCT))" );
#endif //USE_LOG4CXX
            pos += getLengthOfMarker( image, pos );
        }
        else if( marker == "\xff\xc4" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "DHT marker at possition 0x" << std::hex << ( pos - 2 ) << " detected (Define Huffman Table)" );
#endif //USE_LOG4CXX
            pos += getLengthOfMarker( image, pos );
        }
        else if( marker == "\xff\xda" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "SOS marker at possition 0x" << std::hex << ( pos - 2 ) << " detected (Start Of Scan)" );
#endif //USE_LOG4CXX
            pos += getLengthOfMarker( image, pos );
            withinSOS = true;
        }
        else if( marker == "\xff\xd9" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "EOI marker at possition 0x" << std::hex << ( pos - 2 ) << " detected (End Of Image)" );
#endif //USE_LOG4CXX
            seenEOI = true;
        }
        else if( marker == "\xff\xdd" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "DRI marker at possition 0x" << std::hex << ( pos - 2 ) << " detected (Define Restart Interval)" );
#endif //USE_LOG4CXX
            pos += getLengthOfMarker( image, pos );
        }
        else if( marker == "\xff\xde" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "COM marker at possition 0x" << std::hex << ( pos - 2 ) << " detected (Comment)" );
#endif //USE_LOG4CXX
            const int length = getLengthOfMarker( image, pos );

            MarkerShrdPtr marker = std::make_shared< Marker >();
            marker->marker = "\xff\xde";
            marker->content = std::string( &image[pos], length );
            retList.push_back( marker );

            pos += length;
        }
        else
        {
#ifdef USE_LOG4CXX
            LOG4CXX_FATAL( loggerImage, "unknown marker("
                          << std::hex
                          << "0x"
                          << static_cast<int>( marker[0] )
                          << " 0x"
                          << static_cast<int>( marker[1] )
                          << ") at position "
                          << "0x"
                          << ( pos - 2 )
                          << " detected!" );
#endif //USE_LOG4CXX
            return retList;
        }
    }
    
    if( !seenEOI )
    {
#ifdef USE_LOG4CXX
        LOG4CXX_ERROR( loggerImage, "There was no EOI marker. So, the image is probably not correct!" );
#endif //USE_LOG4CXX
        retList.clear();
    }

#ifdef USE_LOG4CXX
    LOG4CXX_INFO( loggerImage, "parse JFIF file and copy markers ... done" );
#endif //USE_LOG4CXX
    
    return retList;    
}

ImageBufferShrdPtr ImageJfif::enrichCompressedImageWithMakers( ImageBufferShrdPtr compressedImage, const ListOfMarkerShrdPtr & markers )
{
    // preparation
    int pos = 0;
    int posNew = 0;
    
    if( !compressedImage )
    {
#ifdef USE_LOG4CXX
        LOG4CXX_ERROR( loggerImage, "compressedImage is a nullptr" );
#endif //USE_LOG4CXX
        return compressedImage;
    }

    if(    ( compressedImage->image == nullptr )
        || ( compressedImage->size == 0 )
      )
    {
#ifdef USE_LOG4CXX
        LOG4CXX_ERROR( loggerImage, "compressedImage->image is a nullptr" );
#endif //USE_LOG4CXX
        return compressedImage;
    }
    
    const char * const image = reinterpret_cast<const char * const>( compressedImage->image );
    const int size           = compressedImage->size;

    // determine the required size for the markers
    int sizeForMarkers = 0;
    for( auto it = markers.begin(); it != markers.end(); ++it )
    {
        if( *it )
        {
            sizeForMarkers += ( (*it)->content.size() + 2 );
        }
    }

    // allocate buffer for the new image
    ImageBufferShrdPtr compressedImageNew = std::make_shared<ImageBuffer>( compressedImage->size + sizeForMarkers );
    char * const imageNew = reinterpret_cast<char * const>( compressedImageNew->image );
    
    // check magic number
    if( std::string( &image[pos], 2 ) != "\xff\xd8" )
    {
#ifdef USE_LOG4CXX
        LOG4CXX_ERROR( loggerImage, "incorrect magic number" );
#endif //USE_LOG4CXX
        return compressedImage;
    }
    std::memcpy( &imageNew[posNew], &image[pos], 2 );
    pos += 2;
    posNew += 2;

    // parse markers of the old image and include new markers
#ifdef USE_LOG4CXX
    LOG4CXX_INFO( loggerImage, "copy markers to new image ..." );
#endif //USE_LOG4CXX

    bool withinSOS = false;
    bool seenEOI = false;
    int  startOfLastSOS = 0 ;
    bool lastWasJfifMarker = false;
    while( pos < size )
    {
        if( lastWasJfifMarker )
        {
            for( auto it = markers.begin(); it != markers.end(); ++it )
            {
                if( *it )
                {
                    std::memcpy( &imageNew[posNew], &((*it)->marker[0]), 2 );
                    posNew += 2;

                    const int sizeOfMarker = (*it)->content.size();
                    std::memcpy( &imageNew[posNew], &((*it)->content[0]), sizeOfMarker );
                    posNew += sizeOfMarker;
                }
            }

            lastWasJfifMarker = false;
        }

        if( withinSOS )
        {
            while( !(    ( static_cast<unsigned char>( image[pos+0] ) == 0xff )
                      && ( static_cast<unsigned char>( image[pos+1] ) != 0x0 )
                    )
                 )
            {
                pos++;
            }
            
            const int lengthSOS = pos - startOfLastSOS;
            std::memcpy( &imageNew[posNew], &image[startOfLastSOS], lengthSOS );
            posNew += lengthSOS;

            withinSOS = false;
            startOfLastSOS = 0;
        }
        
        const std::string marker( &image[pos], 2 );
        pos += 2;
        
        if( marker == "\xff\xe0" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "APP0 (JFIF) marker at possition 0x" << std::hex << ( pos - 2 ) << " detected" );
#endif //USE_LOG4CXX
            const int length = getLengthOfMarker( image, pos );
            std::memcpy( &imageNew[posNew], &image[pos - 2], length + 2 );
            pos += length;
            posNew += ( length + 2 );
            lastWasJfifMarker = true;
        }
        else if( marker == "\xff\xe1" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "APP1 (EXIF / XMP) marker at possition 0x" << std::hex << ( pos - 2 ) << " detected" );
#endif //USE_LOG4CXX
            const int length = getLengthOfMarker( image, pos );
            pos += length;
        }
        else if( marker == "\xff\xe2" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "APP2 (ICC) marker at possition 0x" << std::hex << ( pos - 2 ) << " detected" );
#endif //USE_LOG4CXX
            const int length = getLengthOfMarker( image, pos );
            pos += length;
        }
        else if( marker == "\xff\xe3" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "APP3 (?) marker at possition 0x" << std::hex << ( pos - 2 ) << " detected" );
#endif //USE_LOG4CXX
            const int length = getLengthOfMarker( image, pos );
            pos += length;
        }
        else if( marker == "\xff\xe4" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "APP4 (?) marker at possition 0x" << std::hex << ( pos - 2 ) << " detected" );
#endif //USE_LOG4CXX
            const int length = getLengthOfMarker( image, pos );
            pos += length;
        }
        else if( marker == "\xff\xe5" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "APP5 (?) marker at possition 0x" << std::hex << ( pos - 2 ) << " detected" );
#endif //USE_LOG4CXX
            const int length = getLengthOfMarker( image, pos );
            pos += length;
        }
        else if( marker == "\xff\xe6" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "APP6 (?) marker at possition 0x" << std::hex << ( pos - 2 ) << " detected" );
#endif //USE_LOG4CXX
            const int length = getLengthOfMarker( image, pos );
            pos += length;
        }
        else if( marker == "\xff\xe7" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "APP7 (?) marker at possition 0x" << std::hex << ( pos - 2 ) << " detected" );
#endif //USE_LOG4CXX
            const int length = getLengthOfMarker( image, pos );
            pos += length;
        }
        else if( marker == "\xff\xe8" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "APP8 (?) marker at possition 0x" << std::hex << ( pos - 2 ) << " detected" );
#endif //USE_LOG4CXX
            const int length = getLengthOfMarker( image, pos );
            pos += length;
        }
        else if( marker == "\xff\xe9" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "APP9 (?) marker at possition 0x" << std::hex << ( pos - 2 ) << " detected" );
#endif //USE_LOG4CXX
            const int length = getLengthOfMarker( image, pos );
            pos += length;
        }
        else if( marker == "\xff\xea" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "APP10 (?) marker at possition 0x" << std::hex << ( pos - 2 ) << " detected" );
#endif //USE_LOG4CXX
            const int length = getLengthOfMarker( image, pos );
            pos += length;
        }
        else if( marker == "\xff\xeb" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "APP11 (?) marker at possition 0x" << std::hex << ( pos - 2 ) << " detected" );
#endif //USE_LOG4CXX
            const int length = getLengthOfMarker( image, pos );
            pos += length;
        }
        else if( marker == "\xff\xec" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "APP12 (?) marker at possition 0x" << std::hex << ( pos - 2 ) << " detected" );
#endif //USE_LOG4CXX
            const int length = getLengthOfMarker( image, pos );
            pos += length;
        }
        else if( marker == "\xff\xed" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "APP13 (IPTC) marker at possition 0x" << std::hex << ( pos - 2 ) << " detected" );
#endif //USE_LOG4CXX
            const int length = getLengthOfMarker( image, pos );
            pos += length;
        }
        else if( marker == "\xff\xee" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "APP14 (?) marker at possition 0x" << std::hex << ( pos - 2 ) << " detected" );
#endif //USE_LOG4CXX
            const int length = getLengthOfMarker( image, pos );
            pos += length;
        }
        else if( marker == "\xff\xdb" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "DQT marker at possition 0x" << std::hex << ( pos - 2 ) << " detected (Define Quantization Table)" );
#endif //USE_LOG4CXX
            const int length = getLengthOfMarker( image, pos );
            std::memcpy( &imageNew[posNew], &image[pos - 2], length + 2 );
            pos += length;
            posNew += ( length + 2 );
        }
        else if( marker == "\xff\xc0" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "SOF0 marker at possition 0x" << std::hex << ( pos - 2 ) << " detected (Start Of Frame (baseline DCT))" );
#endif //USE_LOG4CXX
            const int length = getLengthOfMarker( image, pos );
            std::memcpy( &imageNew[posNew], &image[pos - 2], length + 2 );
            pos += length;
            posNew += ( length + 2 );
        }
        else if( marker == "\xff\xc2" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "SOF2 marker at possition 0x" << std::hex << ( pos - 2 ) << " detected (Start Of Frame (progressive DCT))" );
#endif //USE_LOG4CXX
            const int length = getLengthOfMarker( image, pos );
            std::memcpy( &imageNew[posNew], &image[pos - 2], length + 2 );
            pos += length;
            posNew += ( length + 2 );
        }
        else if( marker == "\xff\xc4" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "DHT marker at possition 0x" << std::hex << ( pos - 2 ) << " detected (Define Huffman Table)" );
#endif //USE_LOG4CXX
            const int length = getLengthOfMarker( image, pos );
            std::memcpy( &imageNew[posNew], &image[pos - 2], length + 2 );
            pos += length;
            posNew += ( length + 2 );
        }
        else if( marker == "\xff\xda" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "SOS marker at possition 0x" << std::hex << ( pos - 2 ) << " detected (Start Of Scan)" );
#endif //USE_LOG4CXX
            std::memcpy( &imageNew[posNew], &image[pos - 2], 2 );
            posNew += 2;
            startOfLastSOS = pos;
            pos += getLengthOfMarker( image, pos );
            withinSOS = true;
        }
        else if( marker == "\xff\xd9" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "EOI marker at possition 0x" << std::hex << ( pos - 2 ) << " detected (End Of Image)" );
#endif //USE_LOG4CXX
            std::memcpy( &imageNew[posNew], &image[pos - 2], 2 );
            posNew += 2;
            seenEOI = true;
        }
        else if( marker == "\xff\xdd" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "DRI marker at possition 0x" << std::hex << ( pos - 2 ) << " detected (Define Restart Interval)" );
#endif //USE_LOG4CXX
            const int length = getLengthOfMarker( image, pos );
            std::memcpy( &imageNew[posNew], &image[pos - 2], length + 2 );
            pos += length;
            posNew += ( length + 2 );
        }
        else if( marker == "\xff\xde" )
        {
#ifdef USE_LOG4CXX
            LOG4CXX_DEBUG( loggerImage, "COM marker at possition 0x" << std::hex << ( pos - 2 ) << " detected (Comment)" );
#endif //USE_LOG4CXX
            const int length = getLengthOfMarker( image, pos );
            pos += length;
        }
        else
        {
#ifdef USE_LOG4CXX
            LOG4CXX_FATAL( loggerImage, "unknown marker("
                          << std::hex
                          << "0x"
                          << static_cast<int>( marker[0] )
                          << " 0x"
                          << static_cast<int>( marker[1] )
                          << ") at position "
                          << "0x"
                          << ( pos - 2 )
                          << " detected!" );
#endif //USE_LOG4CXX
            return compressedImage;
        }
    }
    
    if( !seenEOI )
    {
#ifdef USE_LOG4CXX
        LOG4CXX_ERROR( loggerImage, "There was no EOI marker. So, the source image is probably not correct!" );
#endif //USE_LOG4CXX
        return compressedImage;
    }

#ifdef USE_LOG4CXX
    LOG4CXX_INFO( loggerImage, "copy markers to new image ... done" );
#endif //USE_LOG4CXX
    
    return compressedImageNew;
}

} //namespace imageshrink
