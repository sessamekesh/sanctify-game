#ifndef LIB_IGASSET_IGPACK_LOADER_H
#define LIB_IGASSET_IGPACK_LOADER_H

/**
 * Igpack loader - loads an Indigo asset pack, and exposes other promises that
 * can be used to extract individual assets from it.
 */

#include <igasset/draco_decoder.h>
#include <igasset/image_data.h>
#include <igasset/proto/igasset.pb.h>
#include <igasync/promise.h>
#include <igcore/either.h>
#include <igcore/raw_buffer.h>
#include <ignav/detour_navmesh.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>

#include <string>

namespace indigo::asset {

class IgpackLoader {
 public:
  enum class IgpackExtractError {
    FileLoadFailed,
    IgpackParseFailed,
    AssetExtractError,
    ResourceNotFound,
    WrongResourceType,
  };

 public:
  IgpackLoader(const core::RawBuffer& raw_data);
  IgpackLoader(std::string file_name,
               std::shared_ptr<core::TaskList> file_load_task_list);

  // Draco Geo
  typedef core::Either<std::shared_ptr<DracoDecoder>, IgpackExtractError>
      ExtractDracoBufferT;
  typedef std::shared_ptr<core::Promise<ExtractDracoBufferT>>
      ExtractDracoBufferPromiseT;
  ExtractDracoBufferPromiseT extract_draco_geo(
      std::string asset_name,
      std::shared_ptr<core::TaskList> extract_task_list) const;

  // WGSL Source
  typedef core::Either<pb::WgslSource, IgpackExtractError> ExtractWgslShaderT;
  typedef std::shared_ptr<core::Promise<ExtractWgslShaderT>>
      ExtractWgslShaderPromiseT;
  ExtractWgslShaderPromiseT extract_wgsl_shader(
      std::string asset_name,
      std::shared_ptr<core::TaskList> extract_task_list) const;

  // RGB Image
  typedef core::Either<RgbaImage, IgpackExtractError> ExtractRgbaImageDataT;
  typedef std::shared_ptr<core::Promise<ExtractRgbaImageDataT>>
      ExtractRgbaImagePromiseT;
  ExtractRgbaImagePromiseT extract_rgba_image(
      std::string asset_name,
      std::shared_ptr<core::TaskList> extract_task_list) const;

  // Detour navmesh
  typedef core::Either<nav::DetourNavmesh, IgpackExtractError>
      ExtractDetourNavmeshDataT;
  typedef std::shared_ptr<core::Promise<ExtractDetourNavmeshDataT>>
      ExtractDetourNavmeshPromiseT;
  ExtractDetourNavmeshPromiseT extract_detour_navmesh(
      std::string asset_name,
      std::shared_ptr<core::TaskList> extract_task_list) const;

  // Ozz skeleton
  typedef core::Either<ozz::animation::Skeleton, IgpackExtractError>
      ExtractOzzSkeletonT;
  typedef std::shared_ptr<core::Promise<ExtractOzzSkeletonT>>
      ExtractOzzSkeletonPromiseT;
  ExtractOzzSkeletonPromiseT extract_ozz_skeleton(
      std::string asset_name,
      std::shared_ptr<core::TaskList> extract_task_list) const;

  // Ozz animation
  typedef core::Either<ozz::animation::Animation, IgpackExtractError>
      ExtractOzzAnimationT;
  typedef std::shared_ptr<core::Promise<ExtractOzzAnimationT>>
      ExtractOzzAnimationPromiseT;
  ExtractOzzAnimationPromiseT extract_ozz_animation(
      std::string asset_name,
      std::shared_ptr<core::TaskList> extract_task_list) const;

 private:
  std::shared_ptr<
      core::Promise<core::Either<pb::AssetPack, IgpackExtractError>>>
      file_promise_;

  mutable std::map<std::string, ExtractWgslShaderPromiseT> wgsl_promises_;
};

std::string to_string(IgpackLoader::IgpackExtractError error);

}  // namespace indigo::asset

#endif
