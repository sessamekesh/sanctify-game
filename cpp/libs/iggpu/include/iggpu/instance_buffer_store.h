#ifndef LIBS_IGGPU_INCLUDE_IGGPU_INSTANCE_BUFFER_STORE_H
#define LIBS_IGGPU_INCLUDE_IGGPU_INSTANCE_BUFFER_STORE_H

#include <igcore/pod_vector.h>
#include <igcore/vector.h>
#include <iggpu/util.h>
#include <webgpu/webgpu_cpp.h>

#include <functional>
#include <string>

namespace indigo::iggpu {

/**
 * Utility class for caching instance buffers to avoid constantly
 * creating/destroying them.
 *
 * Start a frame with "begin", which resets all instance buffers.
 * For each instance that should be rendered, "record" them as a
 * key:instance pair. This populates underlying contiguous buffers. Once
 * recording is finished, run the "finalize" command, which creates/updates
 * GPU resources with the appropriate instance buffers, and invokes a
 * callback for each key/instance buffer pair.
 *
 * Requirements:
 * 1. KeyT must have a "std::string get_key() const" method
 * 2. KeyT must be copyable (ideally a list of ReadonlyResourceRegistry<>::Key)
 * 3. KeyT must have an "int get_lifetime() const" method
 *    This lifetime method determines how many frames a key can be inactive
 *    before the memory is cleared - this helps reclaim memory use without
 *    completely discarding buffers. A negative number indicates the buffers
 *    should not be kept after finalization.
 * 4. InstanceT must be a POD type that can be stored in a contiguous GPU buffer
 */
template <typename KeyT, typename InstanceT>
class InstanceBufferStore {
 public:
  void begin_frame() {
    for (int i = 0; i < gpu_resource_pairs_.size(); i++) {
      gpu_resource_pairs_[i].instanceCache.resize(0);
    }
  }

  void add_instance(KeyT key, InstanceT instance) {
    get_resource(key).instanceCache.push_back(instance);
  }

  void finalize(
      const wgpu::Device& device,
      std::function<void(const KeyT& key, const wgpu::Buffer& instance_buffer,
                         uint32_t num_instances)>
          cb) {
    for (int i = 0; i < gpu_resource_pairs_.size(); i++) {
      auto& resource = gpu_resource_pairs_[i];
      // Skip dead resources for this frame (no instances added/drawn)
      if (resource.instanceCache.size() == 0) {
        resource.deadFrameCount++;
        continue;
      }

      if (gpu_resource_pairs_[i].gpuResources.buffer == nullptr ||
          resource.gpuResources.bufferCapacity <
              resource.instanceCache.size()) {
        // Case 1: create a new instance buffer on the GPU
        resource.gpuResources.buffer = iggpu::buffer_from_data(
            device, resource.instanceCache, wgpu::BufferUsage::Vertex);
        resource.gpuResources.bufferCapacity = resource.instanceCache.size();
      } else {
        // Case 2: re-use the existing buffer, uploading new data
        device.GetQueue().WriteBuffer(resource.gpuResources.buffer, 0,
                                      &resource.instanceCache[0],
                                      resource.instanceCache.raw_size());
      }

      cb(resource.key, resource.gpuResources.buffer,
         resource.instanceCache.size());
    }

    // Delete all resources that have gone too long without being used. This
    //  includes any resources that were marked as needing to be deleted after
    //  no dead frames.
    // Must be done outside of above loop because some resources may mark
    //  themselves as -1 right off the bat
    for (int i = 0; i < gpu_resource_pairs_.size(); i++) {
      if (gpu_resource_pairs_[i].deadFrameCount >=
          gpu_resource_pairs_[i].frameLifetime) {
        gpu_resource_pairs_.delete_at(i--, false);
      }
    }
  }

 private:
  /** GPU resource and capacity metadata */
  struct GpuInstanceBufferRecord {
    wgpu::Buffer buffer;
    int bufferCapacity;
  };

  /** Entry in a list of resources */
  struct IBKeyValuePair {
    std::string keyString;
    KeyT key;
    int frameLifetime;
    int deadFrameCount;
    GpuInstanceBufferRecord gpuResources;
    indigo::core::PodVector<InstanceT> instanceCache;
  };

  indigo::core::Vector<IBKeyValuePair> gpu_resource_pairs_;

 private:
  IBKeyValuePair& get_resource(KeyT key) {
    std::string key_string = key.get_key();
    for (int i = 0; i < gpu_resource_pairs_.size(); i++) {
      if (gpu_resource_pairs_[i].keyString == key_string) {
        return gpu_resource_pairs_[i];
      }
    }

    gpu_resource_pairs_.push_back(
        IBKeyValuePair{key_string, key, key.get_lifetime(), 0,
                       GpuInstanceBufferRecord{nullptr, 0},
                       indigo::core::PodVector<InstanceT>(4)});
    return gpu_resource_pairs_.last();
  }
};

}  // namespace indigo::iggpu

#endif
