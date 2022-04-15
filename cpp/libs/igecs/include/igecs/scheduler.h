#ifndef LIBS_IGECS_INCLUDE_IGECS_SCHEDULER_H
#define LIBS_IGECS_INCLUDE_IGECS_SCHEDULER_H

#include <igasync/promise.h>
#include <igcore/pod_vector.h>
#include <igcore/vector.h>
#include <igecs/world_view.h>

#include <chrono>

namespace indigo::igecs {

/**
 * ECS scheduler class - used to create a schedule graph and perform execution
 *  of ECS systems concurrently.
 */
class Scheduler {
 public:
  class Builder;

  /** Individual node on the scheduler object */
  class Node {
   public:
    friend class Scheduler;

    Node();

    struct NodeId {
      uint32_t id;

      bool operator==(const NodeId& o) const { return id == o.id; }
      bool operator<(const NodeId& o) const { return id < o.id; }
    };

    class Builder {
     public:
      friend class Node;
      friend class Scheduler;
      friend class Builder;

      Builder& main_thread_only();
      Builder& with_decl(WorldView::Decl decl);
      Builder& depends_on(const Node& node);

      /** Callback consumes a WorldView, and returns an EmptyPromiseRsl */
      [[nodiscard]] Node build(
          std::function<std::shared_ptr<indigo::core::Promise<
              indigo::core::EmptyPromiseRsl>>(WorldView* wv)>
              cb);

     private:
      Builder(NodeId node_id, Scheduler::Builder& b);

      bool is_built_;
      NodeId node_id_;
      bool is_main_thread_only_;
      WorldView::Decl world_view_decl_;
      indigo::core::PodVector<NodeId> dependency_ids_;
      Scheduler::Builder& b_;
    };

    std::shared_ptr<indigo::core::Promise<indigo::core::EmptyPromiseRsl>>
    schedule(entt::registry* world,
             std::shared_ptr<indigo::core::TaskList> main_thread,
             std::shared_ptr<indigo::core::TaskList> any_thread);

   private:
    Node(WorldView::Decl wv_decl, bool main_thread_only, NodeId id,
         indigo::core::PodVector<NodeId> dependency_ids,
         std::function<std::shared_ptr<indigo::core::Promise<
             indigo::core::EmptyPromiseRsl>>(WorldView* wv)>
             cb);

    NodeId id_;
    bool main_thread_only_;
    WorldView::Decl wv_decl_;
    std::function<std::shared_ptr<
        indigo::core::Promise<indigo::core::EmptyPromiseRsl>>(WorldView* wv)>
        cb_;
    indigo::core::PodVector<NodeId> dependency_ids_;
  };

  /** Builder for the scheduler */
  class Builder {
   public:
    friend class Scheduler;
    friend class Node;
    friend class Builder;
    Builder();

    /** What is the longest time spinning is allowed before crashing the app? */
    Builder& max_spin_time(std::chrono::high_resolution_clock::duration dt);

    [[nodiscard]] Node::Builder add_node();
    [[nodiscard]] Scheduler build();

   private:
    indigo::core::Vector<Node> nodes_;
    std::chrono::high_resolution_clock::duration max_spin_time_;
    uint32_t next_node_id_;
  };

  void execute(std::shared_ptr<indigo::core::TaskList> any_thread_task_list,
               entt::registry* world);

 private:
  Scheduler(Builder b);

  /** Return true if and only if a eventually depends on b */
  static bool has_strict_dep(const indigo::core::Vector<Node>& nodes,
                             Node::NodeId a, Node::NodeId b);

  std::chrono::high_resolution_clock::duration max_spin_time_;
  indigo::core::Vector<Node> nodes_;

  // TODO (sessamekesh): Store nodes in a DAG for convenience in scheduling
};

}  // namespace indigo::igecs

#endif
