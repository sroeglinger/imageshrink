
#ifndef ENUM_PIXELFORMAT_H_
#define ENUM_PIXELFORMAT_H_

struct PixelFormat
{
    enum VALUE
    {
        UNKNOWN,
        RGB,
        BGR,
        YCbCr,
        RGBX,
        BGRX,
        XBGR,
        XRGB,
        GRAY,
        RGBA,
        BGRA,
        ABGR,
        ARGB,
        CMYK
    };

    static const char * const toString( VALUE value )
    {
        switch( value )
        {
            case UNKNOWN: return "UNKNOWN";
            case RGB:     return "RGB";
            case BGR:     return "BGR";
            case YCbCr:   return "YCbCr";
            case RGBX:    return "RGBX";
            case BGRX:    return "BGRX";
            case XBGR:    return "XBGR";
            case XRGB:    return "XRGB";
            case GRAY:    return "GRAY";
            case RGBA:    return "RGBA";
            case BGRA:    return "BGRA";
            case ABGR:    return "ABGR";
            case ARGB:    return "ARGB";
            case CMYK:    return "CMYK";
            default:      return "PixelFormat ???";
        }
    }

    static const int channelsPerPixel( VALUE value )
    {
        switch( value )
        {
            case UNKNOWN: return 0;
            case RGB:     return 3;
            case BGR:     return 3;
            case YCbCr:   return 3;
            case RGBX:    return 4;
            case BGRX:    return 4;
            case XBGR:    return 4;
            case XRGB:    return 4;
            case GRAY:    return 1;
            case RGBA:    return 4;
            case BGRA:    return 4;
            case ABGR:    return 4;
            case ARGB:    return 4;
            case CMYK:    return 4;
            default:      return 0;
        }
    }
};

#endif // ENUM_PIXELFORMAT_H_
