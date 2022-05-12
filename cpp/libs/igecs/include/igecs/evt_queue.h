#ifndef LIBS_IGECS_INCLUDE_IGECS_EVT_QUEUE_H
#define LIBS_IGECS_INCLUDE_IGECS_EVT_QUEUE_H

#include <igasync/concurrent_queue.h>

namespace indigo::igecs {

template <typename T>
struct CtxEventQueue {
  moodycamel::ConcurrentQueue<T> queue;
};

}  // namespace indigo::igecs

#endif
