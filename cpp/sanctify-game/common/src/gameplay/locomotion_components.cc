#include <sanctify-game-common/gameplay/locomotion_components.h>

using namespace sanctify;
using namespace component;

void TravelToLocation::serialize(const TravelToLocation& v,
                                 pb::TravelToLocationRequest* o) {
  o->set_x(v.Target.x);
  o->set_y(v.Target.y);
}

void TravelToLocation::deserialize(const pb::TravelToLocationRequest& v,
                                   TravelToLocation* o) {
  o->Target.x = v.x();
  o->Target.y = v.y();
}