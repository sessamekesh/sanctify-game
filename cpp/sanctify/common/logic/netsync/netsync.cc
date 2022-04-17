#include "netsync.h"

using namespace sanctify;
using namespace logic;

bool NetSyncId::operator==(const NetSyncId& o) const { return id == o.id; }

bool NetSyncId::operator<(const NetSyncId& o) const { return id < o.id; }
