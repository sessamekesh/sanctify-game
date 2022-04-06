#include <render/terrain/terrain_instance_buffer_store.h>

using namespace sanctify;
using namespace terrain_pipeline;

std::string TerrainInstanceBufferStore::instancing_key(
    ReadonlyResourceRegistry<terrain_pipeline::TerrainGeo>::Key geo_key,
    ReadonlyResourceRegistry<terrain_pipeline::MaterialPipelineInputs>::Key
        mat_key) {
  return std::to_string(geo_key.get_raw_key()) + ":" +
         std::to_string(mat_key.get_raw_key());
}

wgpu::Buffer TerrainInstanceBufferStore::get_instance_buffer(
    const wgpu::Device& device,
    ReadonlyResourceRegistry<terrain_pipeline::TerrainGeo>::Key geo_key,
    ReadonlyResourceRegistry<terrain_pipeline::MaterialPipelineInputs>::Key
        mat_key,
    const indigo::core::PodVector<
        terrain_pipeline::TerrainMatWorldInstanceData>& raw_data,
    int max_dead_frame_lifetime) {
  auto key = TerrainInstanceBufferStore::instancing_key(geo_key, mat_key);

  for (int i = 0; i < records_.size(); i++) {
    if (records_[i].key == key) {
      records_[i].record.deadFrameCount = 0;
      records_[i].record.gpuBuffer.update_index_data(device, raw_data);
      return records_[i].record.gpuBuffer.InstanceBuffer;
    }
  }

  // Record was not found, create it!
  InstanceBufferRecord new_record{
      max_dead_frame_lifetime, 0,
      terrain_pipeline::TerrainMatWorldInstanceBuffer(device, raw_data)};
  records_.push_back({key, std::move(new_record)});
  return records_[records_.size() - 1].record.gpuBuffer.InstanceBuffer;
}

void TerrainInstanceBufferStore::mark_frame_and_cleanup_dead_buffers() {
  for (int i = 0; i < records_.size(); i++) {
    if (records_[i].record.deadFrameCount++ >=
        records_[i].record.frameLifetime) {
      records_.erase(records_.begin() + i--);
    }
  }
}
