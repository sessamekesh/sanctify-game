#include <converters/assimp_animation_processor.h>
#include <igasset/proto_converters.h>
#include <igcore/log.h>
#include <igcore/raw_buffer.h>
#include <igcore/vector.h>
#include <ozz/animation/offline/animation_builder.h>
#include <ozz/animation/offline/animation_optimizer.h>
#include <ozz/animation/offline/raw_animation.h>
#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/animation/offline/skeleton_builder.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/io/archive.h>

#include <glm/glm.hpp>
#include <queue>

using namespace indigo;
using namespace core;
using namespace igpackgen;

namespace {
const char* kLogLabel = "AssimpAnimationProcessor";
}

bool AssimpAnimationProcessor::preload_animation_bones(
    const pb::AssimpExtractAnimationToOzz& action, FileCache& file_cache,
    AssimpSceneCache& assimp_scene_cache) {
  std::string file_name = action.input_file_path();
  std::string igname = action.skeleton_igasset_name();

  auto opt_scene = assimp_scene_cache.load_scene(file_cache, file_name);
  if (opt_scene.is_empty()) {
    Logger::err(kLogLabel) << "Failed to load scene from file " << file_name;
    return false;
  }

  const aiScene* scene = opt_scene.get()->Scene;

  aiAnimation* animation = nullptr;
  for (int i = 0; i < scene->mNumAnimations; i++) {
    if (action.assimp_animation_name() ==
        scene->mAnimations[i]->mName.C_Str()) {
      animation = scene->mAnimations[i];
      break;
    }
  }
  if (animation == nullptr) {
    Logger::err(kLogLabel) << "Failed to load animation "
                           << action.assimp_animation_name() << " from file "
                           << file_name;
    return false;
  }

  std::set<std::string> bone_names;
  for (int i = 0; i < animation->mNumChannels; i++) {
    const aiNodeAnim* channel = animation->mChannels[i];

    bone_names.insert(channel->mNodeName.C_Str());
  }

  auto it = skeleton_bones_.find(igname);
  if (it == skeleton_bones_.end()) {
    skeleton_bones_.emplace(igname, SkeletonMetadata{bone_names, ""});
    return true;
  }

  if (it->second.boneNames != bone_names) {
    Logger::err(kLogLabel) << "Bone name mismatch (found in animation) "
                           << igname;
    return false;
  }

  return true;
}

bool AssimpAnimationProcessor::validate_bones_exist(
    const pb::AssimpExtractSkeletonToOzz& action, FileCache& file_cache,
    AssimpSceneCache& assimp_scene_cache) {
  std::string file_name = action.input_file_path();

  auto opt_scene = assimp_scene_cache.load_scene(file_cache, file_name);
  if (opt_scene.is_empty()) {
    Logger::err(kLogLabel) << "Failed to load scene from file " << file_name;
    return false;
  }

  const aiScene* scene = opt_scene.get()->Scene;

  const aiNode* root_node = scene->mRootNode;

  std::string igname = action.skeleton_igasset_name();

  auto it = skeleton_bones_.find(igname);
  if (it == skeleton_bones_.end()) {
    Logger::err(kLogLabel) << "Skeleton could not be verified for " << igname
                           << " - bones not pre-registered from animations!";
    return false;
  }

  const aiNode* current_root = nullptr;

  for (const std::string& bone_name : it->second.boneNames) {
    const aiNode* node = root_node->FindNode(bone_name.c_str());
    if (node == nullptr) {
      Logger::err(kLogLabel) << "Animation required bone " << bone_name
                             << " not found in loaded scene file";
      return false;
    }

    if (current_root == nullptr) {
      current_root = node;
      continue;
    }

    if (current_root->FindNode(bone_name.c_str()) == nullptr) {
      // Bone is not found relative to current root - make sure that the new
      //  node can find the current root and set it as root. If this can't
      //  happen, it's because both are sub-trees on the real root - find the
      //  real root!
      // This logic assumes that the real root will eventually come up, which
      //  should absolutely be true!
      if (node->FindNode(current_root->mName)) {
        current_root = node;
      }
    }
  }

  if (current_root == nullptr) {
    Logger::err(kLogLabel) << "No root node could be found!";
    return false;
  }

  if (it->second.rootName == "") {
    it->second.rootName = current_root->mName.C_Str();
  } else if (it->second.rootName != current_root->mName.C_Str()) {
    Logger::err(kLogLabel) << "Unexpected root name "
                           << current_root->mName.C_Str() << ", expected "
                           << it->second.rootName;
    return false;
  }

  // Validated!
  return true;
}

bool AssimpAnimationProcessor::export_skeleton(
    asset::pb::AssetPack& output_asset_pack,
    const pb::AssimpExtractSkeletonToOzz& action, FileCache& file_cache,
    AssimpSceneCache& assimp_scene_cache) {
  //
  // Step 1: Find the root node of the scene referenced by the input action
  //
  std::string file_name = action.input_file_path();

  auto opt_scene = assimp_scene_cache.load_scene(file_cache, file_name);
  if (opt_scene.is_empty()) {
    Logger::err(kLogLabel) << "Failed to load scene from file " << file_name;
    return false;
  }

  const aiScene* scene = opt_scene.get()->Scene;

  auto it = skeleton_bones_.find(action.skeleton_igasset_name());
  if (it == skeleton_bones_.end()) {
    Logger::err(kLogLabel) << "Bones for skeleton "
                           << action.skeleton_igasset_name()
                           << " were not pre-registered!";
    return false;
  }
  const std::set<std::string>& bone_names = it->second.boneNames;
  const std::string& root_bone_name = it->second.rootName;

  const aiNode* root_node = scene->mRootNode->FindNode(root_bone_name.c_str());

  if (root_node == nullptr) {
    Logger::err(kLogLabel) << "Failed to find root node \"" << root_bone_name
                           << "\"";
    return false;
  }

  //
  // Step 2: Build the cumulative root transformation so that the data fed to
  //  Ozz is correct relative to the skeleton root instead of the file root
  //
  aiMatrix4x4 root_transform = root_node->mTransformation;
  for (const aiNode* n = root_node->mParent; n != nullptr; n = n->mParent) {
    root_transform = n->mTransformation * root_transform;
  }

  //
  // Step 3: traverse the Assimp scene graph and add all child nodes to the
  //  skeleton builder.
  //
  struct ConstructionNode {
    const aiNode* node;
    ozz::animation::offline::RawSkeleton::Joint& joint;
    aiMatrix4x4 transform;
  };

  ozz::animation::offline::RawSkeleton raw_skeleton;
  raw_skeleton.roots.resize(1);

  std::queue<ConstructionNode> remaining_nodes;
  remaining_nodes.push({root_node, raw_skeleton.roots[0], root_transform});

  while (!remaining_nodes.empty()) {
    ConstructionNode next = remaining_nodes.front();
    remaining_nodes.pop();

    if (bone_names.count(next.node->mName.C_Str()) > 0) {
      next.joint.name = ozz::string(next.node->mName.C_Str());

      aiVector3D position, scale;
      aiQuaternion rotation;
      next.transform.Decompose(scale, rotation, position);
      next.joint.transform.translation =
          ozz::math::Float3(position.x, position.y, position.z);
      next.joint.transform.rotation =
          ozz::math::Quaternion(rotation.x, rotation.y, rotation.z, rotation.w);
      next.joint.transform.scale = ozz::math::Float3(scale.x, scale.y, scale.z);
    }

    for (int i = 0; i < next.node->mNumChildren; i++) {
      const aiNode* next_node = next.node->mChildren[i];
      if (bone_names.count(next_node->mName.C_Str()) > 0) {
        next.joint.children.push_back(
            ozz::animation::offline::RawSkeleton::Joint{});
      }
    }

    int cidx = 0;
    for (int i = 0; i < next.node->mNumChildren; i++) {
      const aiNode* next_node = next.node->mChildren[i];
      if (bone_names.count(next_node->mName.C_Str()) > 0) {
        remaining_nodes.push({next_node, next.joint.children[cidx++],
                              next_node->mTransformation});
      }
    }
  }

  //
  // Step 4: Construct the skeleton using the Ozz offline library
  //
  ozz::animation::offline::SkeletonBuilder skeleton_builder;

  if (!raw_skeleton.Validate()) {
    Logger::err(kLogLabel) << "Raw skeleton validation failed for "
                           << action.skeleton_igasset_name();
    return false;
  }

  auto skeleton = skeleton_builder(raw_skeleton);
  if (skeleton == nullptr) {
    Logger::err(kLogLabel) << "Failed to construct raw skeleton "
                           << action.skeleton_igasset_name()
                           << " into runtime skeleton";
    return false;
  }

  ozz::io::MemoryStream mem_stream;
  ozz::io::OArchive archive(&mem_stream);
  archive << *skeleton;

  //
  // Step 5: save off for later if the skeleton is used in animations defined
  //  later in this file.
  //
  built_skeletons_[action.skeleton_igasset_name()] = std::move(skeleton);

  RawBuffer raw_buffer(mem_stream.Size());
  mem_stream.Seek(0, ozz::io::MemoryStream::kSet);
  mem_stream.Read(reinterpret_cast<void*>(raw_buffer.get()), mem_stream.Size());

  asset::pb::SingleAsset* new_asset = output_asset_pack.add_assets();
  asset::pb::OzzSkeletonDef* pb_skeleton =
      new_asset->mutable_ozz_skeleton_def();
  new_asset->set_name(action.skeleton_igasset_name());
  pb_skeleton->set_ozz_data(raw_buffer.get(), raw_buffer.size());

  return true;
}

bool AssimpAnimationProcessor::export_animation(
    asset::pb::AssetPack& output_asset_pack,
    const pb::AssimpExtractAnimationToOzz& action, FileCache& file_cache,
    AssimpSceneCache& assimp_scene_cache) {
  //
  // Step 1: Find the root node of the scene referenced by the input action
  //
  std::string file_name = action.input_file_path();

  auto opt_scene = assimp_scene_cache.load_scene(file_cache, file_name);
  if (opt_scene.is_empty()) {
    Logger::err(kLogLabel) << "Failed to load scene from file " << file_name;
    return false;
  }

  const aiScene* scene = opt_scene.get()->Scene;

  aiAnimation* animation = nullptr;
  for (int i = 0; i < scene->mNumAnimations; i++) {
    if (action.assimp_animation_name() ==
        scene->mAnimations[i]->mName.C_Str()) {
      animation = scene->mAnimations[i];
      break;
    }
  }
  if (animation == nullptr) {
    Logger::err(kLogLabel) << "Failed to load animation "
                           << action.assimp_animation_name() << " from file "
                           << file_name;
    return false;
  }

  //
  // Step 2: Find the skeleton to which this animation will be applied, and
  //  prepare mapping from joint name to joint index
  //
  auto skeleton_it = built_skeletons_.find(action.skeleton_igasset_name());
  if (skeleton_it == built_skeletons_.end()) {
    Logger::err(kLogLabel) << "Failed to load skeleton "
                           << action.skeleton_igasset_name()
                           << ", cannot construct animation "
                           << action.assimp_animation_name();
    return false;
  }
  const auto& skeleton = skeleton_it->second;

  std::unordered_map<std::string, uint32_t> name_to_index;
  {
    auto joint_names = skeleton->joint_names();
    for (int i = 0; i < joint_names.size(); i++) {
      const char* joint_name = joint_names[i];
      name_to_index.emplace(joint_name, i);
    }
  }
  std::function<int(std::string)> get_joint_index =
      [&name_to_index](std::string name) -> int {
    auto idx_it = name_to_index.find(name);
    if (idx_it == name_to_index.end()) {
      return -1;
    }
    return idx_it->second;
  };

  //
  // Step 3: Parse out metadata and prepare an Ozz RawAnimation object
  //
  double ticks = animation->mDuration;
  double ticks_per_second = animation->mTicksPerSecond;
  if (ticks_per_second < 0.000001) {
    ticks_per_second = 1.;
  }

  ozz::animation::offline::RawAnimation raw_animation;
  raw_animation.duration = (float)(ticks / ticks_per_second);
  raw_animation.name = action.assimp_animation_name();
  raw_animation.tracks.resize(animation->mNumChannels);

  Vector<std::string> bone_names(animation->mNumChannels);

  for (int channel_idx = 0; channel_idx < animation->mNumChannels;
       channel_idx++) {
    const aiNodeAnim* channel = animation->mChannels[channel_idx];

    std::string bone_name = channel->mNodeName.C_Str();
    bone_names.push_back(bone_name);

    int bone_idx = get_joint_index(bone_name);
    if (bone_idx < 0) {
      Logger::err(kLogLabel)
          << "Bone name " << bone_name
          << " does not exist on skeleton and cannot be used in animation";
      return false;
    }

    for (int pos_key_idx = 0; pos_key_idx < channel->mNumPositionKeys;
         pos_key_idx++) {
      const aiVectorKey& pos_key = channel->mPositionKeys[pos_key_idx];
      double t = pos_key.mTime / animation->mTicksPerSecond;
      const aiVector3D& v = pos_key.mValue;

      raw_animation.tracks[bone_idx].translations.push_back(
          {(float)(t), ozz::math::Float3(v.x, v.y, v.z)});
    }

    for (int rot_key_idx = 0; rot_key_idx < channel->mNumRotationKeys;
         rot_key_idx++) {
      const aiQuatKey& rot_key = channel->mRotationKeys[rot_key_idx];
      double t = rot_key.mTime / animation->mTicksPerSecond;
      const aiQuaternion& q = rot_key.mValue;

      raw_animation.tracks[bone_idx].rotations.push_back(
          {(float)(t), ozz::math::Quaternion(q.x, q.y, q.z, q.w)});
    }

    for (int scl_key_idx = 0; scl_key_idx < channel->mNumScalingKeys;
         scl_key_idx++) {
      const aiVectorKey& scl_key = channel->mScalingKeys[scl_key_idx];
      double t = scl_key.mTime / animation->mTicksPerSecond;
      const aiVector3D& s = scl_key.mValue;

      raw_animation.tracks[bone_idx].scales.push_back(
          {(float)(t), ozz::math::Float3(s.x, s.y, s.z)});
    }
  }

  if (!raw_animation.Validate()) {
    Logger::err(kLogLabel) << "Validation failed for ozz raw animation \""
                           << action.animation_igasset_name() << "\"";
    return false;
  }

  // So go back to the idea of having skeletons built based on what are in the
  //  animations? And then reconcile the bones later I suppose?

  ozz::animation::offline::RawAnimation out_animation;
  bool is_optimized = false;
  if (action.has_optimization_parameters()) {
    ozz::animation::offline::AnimationOptimizer optimizer;

    const pb::AssimpExtractAnimationToOzz_OptimizationParameters&
        optimization_params = action.optimization_parameters();
    if (optimization_params.default_error_measurement_distance() > 0.f) {
      optimizer.setting.distance =
          optimization_params.default_error_measurement_distance();
    }
    if (optimization_params.default_error_tolerance() > 0.f) {
      optimizer.setting.tolerance =
          optimization_params.default_error_tolerance();
    }

    for (int i = 0;
         i < optimization_params.joint_optimization_parameters_size(); i++) {
      const pb::
          AssimpExtractAnimationToOzz_OptimizationParameters_JointOptimizationParameter&
              joint_param =
                  optimization_params.joint_optimization_parameters(i);

      ozz::animation::offline::AnimationOptimizer::Setting joint_setting{};
      joint_setting.distance = joint_param.error_measurement_distance();
      joint_setting.tolerance = joint_param.error_tolerance();

      int joint_idx = get_joint_index(joint_param.joint_name());

      if (joint_idx == -1) {
        Logger::err(kLogLabel)
            << "Skipping joint override for joint \""
            << joint_param.joint_name()
            << "\" - no joint with that name found in provided skeleton";
      } else {
        optimizer.joints_setting_override[joint_idx] = joint_setting;
      }
    }

    if (optimizer(raw_animation, *skeleton_it->second, &out_animation)) {
      is_optimized = true;
    } else {
      Logger::err(kLogLabel)
          << "Failed to optimize animation " << action.animation_igasset_name()
          << ", falling back to unoptimized animation";
    }
  }

  ozz::animation::offline::AnimationBuilder builder;
  auto runtime_animation =
      builder(is_optimized ? out_animation : raw_animation);
  if (runtime_animation == nullptr) {
    Logger::err(kLogLabel) << "Failed to generate animation "
                           << action.animation_igasset_name() << "!";
    return false;
  }

  ozz::io::MemoryStream mem_stream;
  ozz::io::OArchive archive(&mem_stream);
  archive << *runtime_animation;

  RawBuffer raw_buffer(mem_stream.Size());
  mem_stream.Seek(0, ozz::io::MemoryStream::kSet);
  mem_stream.Read(reinterpret_cast<void*>(raw_buffer.get()), mem_stream.Size());

  auto* new_asset = output_asset_pack.add_assets();
  auto* pb_animation = new_asset->mutable_ozz_animation_def();
  new_asset->set_name(action.animation_igasset_name());
  pb_animation->set_data(raw_buffer.get(), raw_buffer.size());

  for (int i = 0; i < bone_names.size(); i++) {
    pb_animation->add_ozz_bone_names(bone_names[i]);
  }

  return true;
}