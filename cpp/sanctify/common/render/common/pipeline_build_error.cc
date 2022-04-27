#include "pipeline_build_error.h"

using namespace sanctify;
using namespace render;

std::string render::to_string(const PipelineBuildError& e) {
  switch (e) {
    case PipelineBuildError::VsLoadError:
      return "VsLoadError";
    case PipelineBuildError::FsLoadError:
      return "FsLoadError";
    case PipelineBuildError::PipelineBuildError:
      return "PipelineBuildError";
  }
}
