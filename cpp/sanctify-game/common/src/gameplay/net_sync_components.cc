#include <sanctify-game-common/gameplay/net_sync_components.h>

using namespace sanctify;
using namespace component;

bool NetSyncId::operator==(const NetSyncId& o) const { return Id == o.Id; }

bool NetSyncId::operator<(const NetSyncId& o) const { return Id < o.Id; }