# Indigo Asset Pack Generator

Tool that takes an Indigo asset pack plan and uses it to generate Indigo asset packs for use in Indigo apps

## Basic Concepts

Indigo asset packs are stored in `.igpack` files, which contain a binary encoded `AssetPack` proto message
(see proto/igasset.proto). The AssetPack proto contains a list of `SingleAsset` resources, and each
`SingleAsset` can be Draco encoded 3D geometry, PNG/HDR encoded textures, declarations of flat texture data,
OZZ skeletons, Detour navigation meshes, etc.

Plans declared in `.igpack-plan` files containing a text encoded `IgpackGenPlan` protocol buffer (see
proto/igpack-plan.proto) are read into this tool, and this tool produces the igpack files defined in the plan.

One igpack-plan can produce zero or more igpack files.

## CMake integration

Once an igpack-plan file is ready for use, use the `build_igpack` CMake function (defined in
cpp/cmake/build_igpack.cmake) like so:

```cmake
build_ig_asset_pack_plan(
  TARGET_NAME foo_asset
  PLAN resources/foo_asset.igpack-plan
  INDIR ${PROJECT_SOURCE_DIR}
  OUTDIR ${PROJECT_BINARY_DIR}
  INFILES
    ${ASSET_ROOT}/foo_image.png
    ${ASSET_ROOT}/foo_geometry.fbx
  TARGET_OUTPUT_FILES
    ${PROJECT_BINARY_DIR}/generated/foo_low_lod.igpack
    ${PROJECT_BINARY_DIR}/generated/foo_medium_lod.igpack
    ${PROJECT_BINARY_DIR}/generated/foo_high_lod.igpack
)
```