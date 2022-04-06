#ifndef SANCTIFY_GAME_CLIENT_RENDER_TERRAIN_TERRAIN_INSTANCE_BUFFER_STORE_H
#define SANCTIFY_GAME_CLIENT_RENDER_TERRAIN_TERRAIN_INSTANCE_BUFFER_STORE_H

#include <render/terrain/terrain_geo.h>
#include <render/terrain/terrain_pipeline.h>
#include <util/resource_registry.h>

#include <vector>

namespace sanctify::terrain_pipeline {

/**
 * Utility class for caching instance buffers to avoid constantly
 * creating/destroying them.
 *
 * "get_instance_buffer" method is the secret sauce, but also call
 * "mark_Frame_and_cleanup_dead_buffers" each frame to release old memory
 */
class TerrainInstanceBufferStore {
 public:
  static std::string instancing_key(
      ReadonlyResourceRegistry<terrain_pipeline::TerrainGeo>::Key geo_key,
      ReadonlyResourceRegistry<terrain_pipeline::MaterialPipelineInputs>::Key
          mat_key);

  wgpu::Buffer get_instance_buffer(
      const wgpu::Device& device,
      ReadonlyResourceRegistry<terrain_pipeline::TerrainGeo>::Key geo_key,
      ReadonlyResourceRegistry<terrain_pipeline::MaterialPipelineInputs>::Key
          mat_key,
      const indigo::core::PodVector<
          terrain_pipeline::TerrainMatWorldInstanceData>& raw_data,
      int max_dead_frame_lifetime);

  void mark_frame_and_cleanup_dead_buffers();

 private:
  struct InstanceBufferRecord {
    int frameLifetime;
    int deadFrameCount;

    terrain_pipeline::TerrainMatWorldInstanceBuffer gpuBuffer;
  };

  struct KeyValuePair {
    std::string key;
    InstanceBufferRecord record;
  };

  std::vector<KeyValuePair> records_;
};

}  // namespace sanctify::terrain_pipeline

#endif
