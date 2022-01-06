#ifndef LIB_IGASSET_HEIGHTMAP_H
#define LIB_IGASSET_HEIGHTMAP_H

#include <igasset/proto/igasset.pb.h>
#include <igcore/either.h>
#include <igcore/pod_vector.h>
#include <igcore/raw_buffer.h>

#include <glm/glm.hpp>

namespace indigo::asset {

class Heightmap {
 public:
  enum class CreateError {
    ZeroWidthInput,
    ZeroHeightInput,
    NoData,
    DimensionMismatch,
  };

  static core::Either<Heightmap, core::PodVector<CreateError>> Create(
      const pb::HeightmapData& data);

  float height_at(glm::vec2 xz_coord) const;

  const glm::vec3& min() const { return min_; }
  const glm::vec3& max() const { return max_; }

  Heightmap(const Heightmap&) = delete;
  Heightmap& operator=(const Heightmap&) = delete;
  Heightmap(Heightmap&&) = default;
  Heightmap& operator=(Heightmap&&) = default;
  ~Heightmap() = default;

 private:
  Heightmap(core::RawBuffer raw_data, uint32_t raw_data_width,
            uint32_t raw_data_height, glm::vec3 min, glm::vec3 max)
      : raw_data_(std::move(raw_data)),
        raw_data_width_(raw_data_width),
        raw_data_height_(raw_data_height),
        min_(min),
        max_(max) {}

  float sample(uint32_t x, uint32_t z) const;

  core::RawBuffer raw_data_;
  uint32_t raw_data_width_;
  uint32_t raw_data_height_;

  glm::vec3 min_;
  glm::vec3 max_;
};

std::string to_string(const Heightmap::CreateError& error);

}  // namespace indigo::asset

#endif
