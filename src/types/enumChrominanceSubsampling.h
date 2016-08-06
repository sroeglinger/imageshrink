
#ifndef ENUM_CHROMINANCESUBSAMPLING_H_
#define ENUM_CHROMINANCESUBSAMPLING_H_

struct ChrominanceSubsampling
{
    enum VALUE
    {
        UNKNOWN,
        NONE,
        CS_444,
        CS_422,
        CS_420,
        CS_440,
        CS_411,
        Gray
    };

    static const char * const toString( VALUE value )
    {
        switch( value )
        {
            case UNKNOWN: return "UNKNOWN";
            case NONE:    return "NONE";
            case CS_444:  return "CS_444";
            case CS_422:  return "CS_422";
            case CS_420:  return "CS_420";
            case CS_440:  return "CS_440";
            case CS_411:  return "CS_411";
            case Gray:    return "Gray";
            default:      return "ChrominanceSubsampling ???";
        }
    }
};

#endif // ENUM_CHROMINANCESUBSAMPLING_H_
