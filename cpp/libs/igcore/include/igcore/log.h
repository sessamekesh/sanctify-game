#ifndef _INDIGO_CORE_LOG_H_
#define _INDIGO_CORE_LOG_H_

#include <igcore/config.h>

#include <iostream>
#include <sstream>
#include <string>

namespace indigo::core {

/**
 * Logging utility class - has two distinct advantages over std::cout / cerr /
 * clog:
 * - Non-interleaving in threaded environments
 * - May be enabled/disabled via build flag (do not compile logging to
 * production build)
 *
 * Usage:
 * Logger::get() << "Here's something to log" << var << "and whatever else";
 *
 * Logger::get() returns a LoggerWrapper, which locks output to this thread. Be
 * careful not to include anything nutty here! The lock should not be long
 * lived!
 *
 * Ideally, this whole file is just optimized right away when the flag to use it
 * is disabled - e.g., any logging statements are downright removed.
 */

class LogWrapper {
 public:
#ifdef IG_ENABLE_LOGGING
  LogWrapper(std::ostream* o, const std::string& obj_name) : o_(o) {
    if (obj_name.size() > 0) {
      ss_ << "[" << obj_name << "] ";
    }
  }

  ~LogWrapper() {
    ss_ << "\r\n";
    *o_ << ss_.str();
  }
#else
  LogWrapper() = default;
  ~LogWrapper() = default;
#endif

  template <typename T>
  LogWrapper& operator<<(T t) {
#ifdef IG_ENABLE_LOGGING
    ss_ << t;
#endif
    return *this;
  }

  // Support std::endl
  LogWrapper& operator<<(std::ostream& (*f)(std::ostream&));

 private:
#ifdef IG_ENABLE_LOGGING
  std::stringstream ss_;
  std::ostream* o_;
#endif
};

class Logger {
 public:
  static inline LogWrapper log(const std::string& obj_name = "") {
#ifdef IG_ENABLE_LOGGING
    return LogWrapper(&std::cout, obj_name);
#else
    return LogWrapper();
#endif
  }

  static inline LogWrapper err(const std::string& obj_name = "") {
#ifdef IG_ENABLE_LOGGING
    return LogWrapper(&std::cerr, obj_name);
#else
    return LogWrapper();
#endif
  }
};

}  // namespace indigo::core

#endif
