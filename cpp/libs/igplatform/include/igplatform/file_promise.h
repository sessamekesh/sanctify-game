#ifndef _LIB_IGPLATFORM_FILE_PROMISE_H_
#define _LIB_IGPLATFORM_FILE_PROMISE_H_

#include <igasync/promise.h>
#include <igcore/either.h>
#include <igcore/raw_buffer.h>

namespace indigo::core {

enum class FileReadError {
  FileNotFound,
  FileNotRead,
};

typedef Either<std::shared_ptr<RawBuffer>, FileReadError> FilePromiseResultT;
typedef Promise<FilePromiseResultT> FilePromiseT;

class FilePromise {
 public:
  static std::string error_log_label(FileReadError err);

  static std::shared_ptr<FilePromiseT> Create(
      const std::string& file_name, const std::shared_ptr<TaskList>& task_list);
};

// TODO (sessamekesh): Continue here, writing the file promise (copy it over...)

}  // namespace indigo::core

#endif
