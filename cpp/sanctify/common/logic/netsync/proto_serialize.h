#ifndef SANCTIFY_COMMON_LOGIC_NETSYNC_PROTO_SERIALIZE_H
#define SANCTIFY_COMMON_LOGIC_NETSYNC_PROTO_SERIALIZE_H

#include <common/logic/locomotion/locomotion.h>
#include <common/logic/update_common/tick_time_elapsed.h>
#include <common/pb/common_logic_snapshot.pb.h>
#include <common/pb/primitives.pb.h>

#include <glm/glm.hpp>

/**
 * Collection of static utility methods to serialize/deserialize logical
 *  components from their proto representations. Intended mostly for use
 *  within snapshot/diff logic, but accessible to other users if a case
 *  arises.
 *
 * Includes common types, e.g. glm::vec2, glm::vec3...
 *
 * For compatibility with various conversion hacks, please keep the following
 * structure:
 *
 * void serialize(ProtoT* mut_pb, ValT value); // optionally const ValT& value
 * ValT deserialize(const ProtoT& pb);
 */

namespace sanctify::logic {

//
// Primitives
//

void serialize(common::pb::Vec2* mut_pb, glm::vec2 value);
glm::vec2 deserialize(const common::pb::Vec2& pb);

//
// Context components
//

void serialize(common::pb::CtxSimTimeComponent* mut_pb, CtxSimTime time);
CtxSimTime deserialize(const common::pb::CtxSimTimeComponent& pb);

//
// Components
//

void serialize(common::pb::MapLocationComponent* mut_cb,
               MapLocationComponent val);
MapLocationComponent deserialize(const common::pb::MapLocationComponent& pb);

void serialize(common::pb::OrientationComponent* mut_cb,
               OrientationComponent val);
OrientationComponent deserialize(const common::pb::OrientationComponent& pb);

void serialize(common::pb::NavWaypointListComponent* mut_cb,
               const NavWaypointListComponent& val);
NavWaypointListComponent deserialize(
    const common::pb::NavWaypointListComponent& pb);

void serialize(common::pb::StandardNavigationParamsComponent* mut_cb,
               StandardNavigationParamsComponent val);
StandardNavigationParamsComponent deserialize(
    const common::pb::StandardNavigationParamsComponent& pb);

}  // namespace sanctify::logic

#endif
