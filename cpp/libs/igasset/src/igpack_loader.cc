#include <igasset/igpack_loader.h>
#include <igplatform/file_promise.h>

using namespace indigo;
using namespace asset;

namespace {
const char* kLogLabel = "IgpackLoader";
}

std::string asset::to_string(IgpackLoader::IgpackExtractError error) {
#ifdef IG_ENABLE_LOGGING
  switch (error) {
    case IgpackLoader::IgpackExtractError::AssetExtractError:
      return "AssetExtractError";
    case IgpackLoader::IgpackExtractError::FileLoadFailed:
      return "FileLoadFailed";
    case IgpackLoader::IgpackExtractError::IgpackParseFailed:
      return "IgpackParseFailed";
    case IgpackLoader::IgpackExtractError::ResourceNotFound:
      return "ResourceNotFound";
    case IgpackLoader::IgpackExtractError::WrongResourceType:
      return "WrongResourceType";
    default:
      return "<<UNKNOWN>>";
  }
#else
  return "";
#endif
}

IgpackLoader::IgpackLoader(
    std::string file_name,
    std::shared_ptr<core::TaskList> file_load_task_list) {
  file_promise_ =
      core::FilePromise::Create(file_name, file_load_task_list)
          ->then<core::Either<pb::AssetPack, IgpackExtractError>>(
              [](const auto& rsl)
                  -> core::Either<pb::AssetPack, IgpackExtractError> {
                if (rsl.is_right()) {
                  return core::right(IgpackExtractError::FileLoadFailed);
                }

                const auto& buffer = rsl.get_left();

                asset::pb::AssetPack asset_pack;
                if (!asset_pack.ParseFromArray(buffer->get(), buffer->size())) {
                  return core::right(IgpackExtractError::IgpackParseFailed);
                }

                return core::left(std::move(asset_pack));
              },
              file_load_task_list);
}

IgpackLoader::IgpackLoader(const core::RawBuffer& raw_buffer) {
  file_promise_ =
      core::Promise<core::Either<pb::AssetPack, IgpackExtractError>>::create();

  asset::pb::AssetPack asset_pack;
  if (!asset_pack.ParseFromArray(raw_buffer.get(), raw_buffer.size())) {
    file_promise_->resolve(core::right(IgpackExtractError::IgpackParseFailed));
  }

  file_promise_->resolve(core::left(std::move(asset_pack)));
}

IgpackLoader::ExtractDracoBufferPromiseT IgpackLoader::extract_draco_geo(
    std::string asset_name,
    std::shared_ptr<core::TaskList> extract_task_list) const {
  return file_promise_->then<ExtractDracoBufferT>(
      [asset_name](const core::Either<pb::AssetPack, IgpackExtractError>& rsl)
          -> ExtractDracoBufferT {
        if (rsl.is_right()) {
          return core::right(rsl.get_right());
        }

        const auto& asset_pack = rsl.get_left();
        for (int i = 0; i < asset_pack.assets_size(); i++) {
          const auto& asset = asset_pack.assets(i);
          if (asset.name() != asset_name) {
            continue;
          }

          if (!asset.has_draco_geo()) {
            core::Logger::err(kLogLabel)
                << "Resource " << asset_name << " is not a Draco resource";
            return core::right(IgpackExtractError::WrongResourceType);
          }

          auto decoder = std::make_shared<asset::DracoDecoder>();
          const std::string& data = asset.draco_geo().data();
          core::RawBuffer buffer((uint8_t*)&data[0], data.size(), false);
          auto rsl = decoder->decode(buffer);
          if (rsl != asset::DracoDecoderResult::Ok) {
            core::Logger::err(kLogLabel)
                << "Could not process Draco asset in " << asset_name << " - "
                << ::to_string(rsl);
            return core::right(IgpackExtractError::AssetExtractError);
          }

          return core::left(std::move(decoder));
        }

        // Asset was not found if returning here
        return core::right(IgpackExtractError::ResourceNotFound);
      },
      extract_task_list);
}

IgpackLoader::ExtractWgslShaderPromiseT IgpackLoader::extract_wgsl_shader(
    std::string asset_name,
    std::shared_ptr<core::TaskList> extract_task_list) const {
  auto existing_promise = wgsl_promises_.find(asset_name);
  if (existing_promise != wgsl_promises_.end()) {
    return existing_promise->second;
  }

  ExtractWgslShaderPromiseT rsl = file_promise_->then<ExtractWgslShaderT>(
      [asset_name](const core::Either<pb::AssetPack, IgpackExtractError>& rsl)
          -> ExtractWgslShaderT {
        if (rsl.is_right()) {
          return core::right(rsl.get_right());
        }

        const auto& asset_pack = rsl.get_left();
        for (int i = 0; i < asset_pack.assets_size(); i++) {
          const auto& asset = asset_pack.assets(i);
          if (asset.name() != asset_name) {
            continue;
          }

          if (!asset.has_wgsl_source()) {
            core::Logger::err(kLogLabel)
                << "Resource " << asset_name << " is not a WGSL source";
            return core::right(IgpackExtractError::WrongResourceType);
          }

          return core::left(asset.wgsl_source());
        }

        core::Logger::err(kLogLabel)
            << "WGSL resource " << asset_name << " not found!";
        for (int i = 0; i < asset_pack.assets_size(); i++) {
          core::Logger::err(kLogLabel)
              << "-- asset " << i << " : " << asset_pack.assets(i).name();
        }
        return core::right(IgpackLoader::IgpackExtractError::ResourceNotFound);
      },
      extract_task_list);
  wgsl_promises_.insert({asset_name, rsl});
  return rsl;
}

IgpackLoader::ExtractRgbaImagePromiseT IgpackLoader::extract_rgba_image(
    std::string asset_name,
    std::shared_ptr<core::TaskList> extract_task_list) const {
  return file_promise_->then<ExtractRgbaImageDataT>(
      [asset_name](const auto& rsl) -> ExtractRgbaImageDataT {
        if (rsl.is_right()) {
          return core::right(rsl.get_right());
        }

        const asset::pb::AssetPack& asset_pack = rsl.get_left();
        for (int i = 0; i < asset_pack.assets_size(); i++) {
          const auto& asset = asset_pack.assets(i);
          if (asset.name() != asset_name) {
            continue;
          }

          if (!asset.has_png_texture_def()) {
            core::Logger::err(kLogLabel)
                << "Resource " << asset_name << " is not a PNG image source";
            return core::right(IgpackExtractError::WrongResourceType);
          }

          auto& png_texture_def = asset.png_texture_def();

          auto rgba_image_rsl = RgbaImage::ParsePNG(png_texture_def.data());
          if (rgba_image_rsl.is_right()) {
            core::Logger::err(kLogLabel)
                << "Resource " << asset_name << " could not be decoded as PNG";
            return core::right(IgpackExtractError::AssetExtractError);
          }

          return core::left(rgba_image_rsl.left_move());
        }

        return core::right(IgpackExtractError::ResourceNotFound);
      },
      extract_task_list);
}