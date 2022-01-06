#include <igcore/log.h>

using namespace indigo;
using namespace core;

LogWrapper& LogWrapper::operator<<(std::ostream& (*f)(std::ostream& s)) {
#ifdef IG_ENABLE_LOGGING
  f(*o_);
#endif
  return *this;
}