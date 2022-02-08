#include <sanctify-game-common/gameplay/locomotion_components.h>

using namespace sanctify;
using namespace component;

bool MapLocation::operator==(const MapLocation& o) const { return XZ == o.XZ; }

bool NavWaypointList::operator==(const NavWaypointList& o) const {
  if (Targets.size() != o.Targets.size()) {
    return false;
  }

  return memcmp(&Targets[0], &o.Targets[0], Targets.raw_size()) == 0;
}

bool StandardNavigationParams::operator==(
    const StandardNavigationParams& o) const {
  return MovementSpeed == o.MovementSpeed;
}