#include <igplatform/file_promise.h>

#include <fstream>

using namespace indigo;
using namespace core;

std::shared_ptr<FilePromiseT> FilePromise::Create(
    const std::string& file_name, const std::shared_ptr<TaskList>& task_list) {
  auto rsl = FilePromiseT::create("ReadFile");

  task_list->add_task(Task::of([rsl, file_name]() {
    std::ifstream fin(file_name, std::ios::binary | std::ios::ate);

    if (!fin) {
      rsl->resolve(core::right(FileReadError::FileNotFound));
      return;
    }

    std::streamsize size = fin.tellg();
    fin.seekg(0, std::ios::beg);

    auto data = std::make_shared<RawBuffer>(size);
    if (!fin.read(reinterpret_cast<char*>(data->get()), size)) {
      rsl->resolve(core::right(FileReadError::FileNotRead));
      return;
    }

    rsl->resolve(core::left(std::move(data)));
  }));

  return rsl;
}