#ifndef LIB_IGASSET_VERTEX_FORMATS_H
#define LIB_IGASSET_VERTEX_FORMATS_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

/**
 * Collection of common vertex formats - not all vertex formats fit
 *  in here (obviously), but common formats should all go in here.
 */

namespace indigo::asset {

struct LegacyPositionNormalVertexData {
  glm::vec3 Position;
  glm::vec3 Normal;
};

struct LegacyTangentBitangentVertexData {
  glm::vec3 Tangent;
  glm::vec3 Bitangent;
};

/**
 * Position+normal information for a vertex - used in almost all 3D lighting
 *  models. Normal information is stored in a quaternion - either a normal
 *  vector (0, 1, 0) or a TBN matrix (identity) can be rotated by this quat
 *  in a fragment shader.
 */
struct PositionNormalVertexData {
  glm::vec3 Position;
  glm::vec4 NormalQuat;
};

/**
 * Texture coordinate information - multiple of these may be attached as inputs
 * to a material / shader.
 */
struct TexcoordVertexData {
  glm::vec2 Texcoord;
};

/**
 * Vertex animation data - encapsulates data used for doing skeletal animation
 */
struct SkeletalAnimationVertexData {
  glm::vec4 BoneWeights;
  glm::uvec4 BoneIndices;
};

/**
 * World position data - extremely common in 3D instanced geometry
 */
struct WorldTransformInstanceData {
  glm::mat4 MatWorld;
};

}  // namespace indigo::asset

#endif
