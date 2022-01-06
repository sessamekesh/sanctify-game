#include <igcore/config.h>
#include <igplatform/file_promise.h>

using namespace indigo;
using namespace core;

std::string FilePromise::error_log_label(FileReadError err) {
#ifdef IG_ENABLE_LOGGING
  switch (err) {
    case FileReadError::FileNotFound:
      return "FileNotFound";
    case FileReadError::FileNotRead:
      return "FileNotRead";
    default:
      return "FileReadError::Unenumerated";
  }
#else
  return "";
#endif
}
