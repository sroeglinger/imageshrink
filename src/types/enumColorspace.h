
#ifndef ENUM_COLORSPACE_H_
#define ENUM_COLORSPACE_H_

struct Colorspace
{
    enum VALUE
    {
        UNKNOWN,
        RGB,
        YCbCr,
        Gray,
        CMYK,
        YCCK
    };

    static const char * const toString( VALUE value )
    {
        switch( value )
        {
            case UNKNOWN:   return "UNKNOWN";
            case RGB:       return "RGB";
            case YCbCr:     return "YCbCr";
            case Gray:      return "Gray";
            case CMYK:      return "CMYK";
            case YCCK:      return "YCCK";
            default:        return "Colorspaces ???";
        }
    }
};

#endif // ENUM_COLORSPACE_H_
