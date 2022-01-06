#include <igasset/image_data.h>
#include <stb/stb_image.h>

using namespace indigo;
using namespace asset;

core::Either<GreyscaleImage, ParseError> GreyscaleImage::ParsePNG(
    const std::string& raw_png_data) {
  int width, height, num_channels;
  auto* img_data =
      stbi_load_from_memory((uint8_t*)&raw_png_data[0], raw_png_data.size(),
                            &width, &height, &num_channels, 1);

  if (img_data == nullptr) {
    return core::right(ParseError::ParseFailed);
  }

  core::PodVector<GreyscalePixel> pixels((size_t)width * (size_t)height);
  for (int row = 0; row < height; row++) {
    for (int col = 0; col < width; col++) {
      auto pixel_index = (row * width + col) * num_channels;
      GreyscalePixel pixel;
      pixel.Color = img_data[pixel_index];
      pixels.push_back(pixel);
    }
  }

  GreyscaleImage image;
  image.Width = width;
  image.Height = height;
  image.Pixels = std::move(pixels);

  return core::left(std::move(image));
}

core::Either<RgbaImage, ParseError> RgbaImage::ParsePNG(
    const std::string& raw_png_data) {
  int width, height, num_channels;
  uint8_t* img_data =
      stbi_load_from_memory((uint8_t*)&raw_png_data[0], raw_png_data.size(),
                            &width, &height, &num_channels, 0);

  if (img_data == nullptr) {
    return core::right(ParseError::ParseFailed);
  }

  core::PodVector<RgbaPixel> pixels((size_t)width * (size_t)height);
  for (int row = 0; row < height; row++) {
    for (int col = 0; col < width; col++) {
      auto pixel_index = (row * width + col) * num_channels;
      RgbaPixel pixel;
      pixel.Color[0] = img_data[pixel_index];
      pixel.Color[1] = (num_channels >= 2) ? img_data[pixel_index + 1] : 0u;
      pixel.Color[2] = (num_channels >= 3) ? img_data[pixel_index + 2] : 0u;
      pixel.Color[3] = (num_channels >= 4) ? img_data[pixel_index + 3] : 255u;

      pixels.push_back(pixel);
    }
  }

  stbi_image_free(img_data);

  RgbaImage image;
  image.Width = width;
  image.Height = height;
  image.Pixels = std::move(pixels);

  return core::left(std::move(image));
}

core::Either<RgbImage, ParseError> RgbImage::ParsePNG(
    const std::string& raw_png_data) {
  int width, height, num_channels;
  auto* img_data =
      stbi_load_from_memory((uint8_t*)&raw_png_data[0], raw_png_data.size(),
                            &width, &height, &num_channels, 1);

  if (img_data == nullptr) {
    return core::right(ParseError::ParseFailed);
  }

  core::PodVector<RgbPixel> pixels((size_t)width * (size_t)height);
  for (int row = 0; row < height; row++) {
    for (int col = 0; col < width; col++) {
      auto pixel_index = (row * width + col) * num_channels;
      RgbPixel pixel;
      pixel.Color[0] = img_data[pixel_index];
      pixel.Color[1] = (num_channels >= 2) ? img_data[pixel_index + 1] : 0u;
      pixel.Color[2] = (num_channels >= 3) ? img_data[pixel_index + 2] : 0u;

      pixels.push_back(pixel);
    }
  }

  RgbImage image;
  image.Width = width;
  image.Height = height;
  image.Pixels = std::move(pixels);

  return core::left(std::move(image));
}