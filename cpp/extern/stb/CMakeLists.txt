add_library(stb
  "include/stb/stb_image.h"
  "src/stb_image.cc" "include/stb/stb_image_resize.h" "include/stb/stb_image_write.h")
TARGET_INCLUDE_DIRECTORIES(stb PUBLIC include)