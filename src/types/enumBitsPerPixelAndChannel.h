
#ifndef ENUM_BITSPERPIXELANDCHANNEL_H_
#define ENUM_BITSPERPIXELANDCHANNEL_H_

struct BitsPerPixelAndChannel
{
    enum VALUE
    {
        UNKNOWN,
        BITS_8,
        BITS_16,
        BITS_32
    };

    static const char * const toString( VALUE value )
    {
        switch( value )
        {
            case UNKNOWN:  return "UNKNOWN";
            case BITS_8:   return "BITS_8";
            case BITS_16:  return "BITS_16";
            case BITS_32:  return "BITS_32";
            default:       return "BitsPerPixelAndChannel ???";
        }
    }

    static const int bytesPerChannel( VALUE value )
    {
        switch( value )
        {
            case UNKNOWN:  return 0;
            case BITS_8:   return 1;
            case BITS_16:  return 2;
            case BITS_32:  return 4;
            default:       return 0;
        }
    }
};

#endif // ENUM_BITSPERPIXELANDCHANNEL_H_
