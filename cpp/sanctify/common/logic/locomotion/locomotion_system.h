#ifndef SANCTIFY_COMMON_LOGIC_LOCOMOTION_LOCOMOTION_SYSTEM_H
#define SANCTIFY_COMMON_LOGIC_LOCOMOTION_LOCOMOTION_SYSTEM_H

#include <igecs/world_view.h>

#include "locomotion.h"

namespace sanctify::logic {

class LocomotionSystem {
 public:
  static const indigo::igecs::WorldView::Decl& decl();
  static void update(indigo::igecs::WorldView* world);
};

}  // namespace sanctify::logic

#endif
