#ifndef IGASSET_IMAGE_DATA_H
#define IGASSET_IMAGE_DATA_H

#include <igcore/either.h>
#include <igcore/pod_vector.h>

#include <cstdint>
#include <glm/glm.hpp>
#include <string>

namespace indigo::asset {

enum class ParseError {
  ParseFailed,
};

struct GreyscalePixel {
  uint8_t Color;
};
struct GreyscaleImage {
  core::PodVector<GreyscalePixel> Pixels;
  uint32_t Width;
  uint32_t Height;

  static core::Either<GreyscaleImage, ParseError> ParsePNG(
      const std::string& raw_png_data);
};

struct RgbPixel {
  uint8_t Color[3];
};
struct RgbImage {
  core::PodVector<RgbPixel> Pixels;
  uint32_t Width;
  uint32_t Height;

  static core::Either<RgbImage, ParseError> ParsePNG(
      const std::string& raw_png_data);
};

struct RgbaPixel {
  uint8_t Color[4];
};
struct RgbaImage {
  core::PodVector<RgbaPixel> Pixels;
  uint32_t Width;
  uint32_t Height;

  static core::Either<RgbaImage, ParseError> ParsePNG(
      const std::string& raw_png_data);
};

}  // namespace indigo::asset

#endif
