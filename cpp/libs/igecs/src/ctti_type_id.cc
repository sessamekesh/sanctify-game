#include <igecs/ctti_type_id.h>

using namespace indigo;
using namespace igecs;

std::atomic_uint32_t CttiTypeId::next_type_id_ = 0ul;
