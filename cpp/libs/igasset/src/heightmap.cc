#include <igasset/heightmap.h>
#include <igcore/log.h>
#include <igcore/math.h>

using namespace indigo;
using namespace asset;

namespace {
const char* kLogLabel = "Igasset::Heightmap";
}

std::string asset::to_string(const Heightmap::CreateError& err) {
  switch (err) {
    case Heightmap::CreateError::NoData:
      return "NoData";
    case Heightmap::CreateError::ZeroHeightInput:
      return "ZeroHeightInput";
    case Heightmap::CreateError::ZeroWidthInput:
      return "ZeroWidthInput";
    case Heightmap::CreateError::DimensionMismatch:
      return "DimensionMismatch";
    default:
      return "<<Unknown Heightmap::CreateError>>";
  }
}

float Heightmap::sample(uint32_t x, uint32_t z) const {
  x = glm::clamp(x, 0u, raw_data_width_ - 1);
  z = glm::clamp(z, 0u, raw_data_height_ - 1);

  uint8_t raw_sample = raw_data_[z * raw_data_width_ + x];

  float t = ((float)raw_sample / 255.f);

  return core::igmath::lerp(t, min_.y, max_.y);
}

float Heightmap::height_at(glm::vec2 xz_coord) const {
  float u = (xz_coord.x - min_.x) / (max_.x - min_.x);
  // TODO (sessamekesh): Should this be top to bottom or bottom to top?
  float v = (xz_coord.y - min_.z) / (max_.z - min_.z);

  u = glm::clamp(u, 0.f, 1.f);
  v = glm::clamp(v, 0.f, 1.f);

  uint32_t l = (uint32_t)glm::floor(u * (raw_data_width_ - 1));
  uint32_t r = (uint32_t)glm::ceil(u * (raw_data_width_ - 1));
  uint32_t t = (uint32_t)glm::ceil(v * (raw_data_height_ - 1));
  uint32_t b = (uint32_t)glm::floor(v * (raw_data_height_ - 1));

  float lr_lerp_t = (u * (raw_data_width_ - 1) - l);
  float top_lerp = core::igmath::lerp(lr_lerp_t, sample(l, t), sample(r, t));
  float bot_lerp = core::igmath::lerp(lr_lerp_t, sample(l, b), sample(r, b));

  float tb_lerp_t = (v * (raw_data_height_ - 1) - b);
  return core::igmath::lerp(tb_lerp_t, bot_lerp, top_lerp);
}

core::Either<Heightmap, core::PodVector<Heightmap::CreateError>>
Heightmap::Create(const pb::HeightmapData& data) {
  core::PodVector<Heightmap::CreateError> errors;

  if (data.data().size() == 0u) {
    errors.push_back(CreateError::NoData);
  }

  if (data.data_width() == 0u) {
    errors.push_back(CreateError::ZeroWidthInput);
  }

  if (data.data_height() == 0u) {
    errors.push_back(CreateError::ZeroHeightInput);
  }

  if (data.data().size() != data.data_width() * data.data_height()) {
    errors.push_back(CreateError::DimensionMismatch);
  }

  if (errors.size() > 0) {
    return core::right(std::move(errors));
  }

  core::RawBuffer raw_data(data.data_width() * data.data_height());
  memcpy(raw_data.get(), &data.data()[0], raw_data.size());

  return core::left(
      Heightmap(std::move(raw_data), data.data_width(), data.data_height(),
                glm::vec3{data.min_x(), data.min_y(), data.min_z()},
                glm::vec3{data.max_x(), data.max_y(), data.max_z()}));
}