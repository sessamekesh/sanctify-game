#ifndef SANCTIFY_COMMON_RENDER_COMMON_PIPELINE_BUILD_ERROR_H
#define SANCTIFY_COMMON_RENDER_COMMON_PIPELINE_BUILD_ERROR_H

#include <string>

namespace sanctify::render {

enum class PipelineBuildError {
  VsLoadError,
  FsLoadError,
  PipelineBuildError,
};

std::string to_string(const PipelineBuildError& e);

}  // namespace sanctify::render

#endif
