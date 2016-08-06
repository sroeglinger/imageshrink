
#ifndef STRINGSHRDPTR_H_
#define STRINGSHRDPTR_H_

// include system headers
#include <string>
#include <memory> // for smart pointer

namespace imageshrink
{

typedef std::shared_ptr<std::string>       stringShrdPtr;
typedef std::shared_ptr<std::string const> stringConstShrdPtr;

} //namespace imageshrink

#endif // STRINGSHRDPTR_H_
