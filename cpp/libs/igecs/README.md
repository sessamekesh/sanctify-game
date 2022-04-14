# IgECS

Wrapper around [EnTT library](https://github.com/skypjack/entt) that provides a few nice concurrency helpers:

(1) `WorldView` class that wraps an EnTT registry and can declare what data will be read/written
(2) `Scheduler` and `ScheduleNode` to organize ECS systems into a runtime DAG

## Intended Usage

WorldView / WorldView::Decl

```cpp
// Create an EnTT registry
entt::registry world;

// For one-off updates (e.g. initialization time), create a "thin" WorldView
auto wv = WorldView::CreateThin(&world);
wv.set_ctx<CtxGlobalFoo>();
SomeInitHelperUtility::setup_globals(&wv);

// WorldViewDecls can be created to describe which WorldView can be created..
class MovementSystem {
  public:
    static WorldView::Decl world_view_decl() {
      WorldView::Decl decl;
      decl.reads<VelocityComponent>();
      decl.writes<PositionComponent>();
      decl.reads_ctx<CtxGameParams>();
      return decl;
    }
};

// ... later queried...
auto d = MovementSystem::world_view_decl();
if (d.writes<PositionComponent>()) {
  // Avoid scheduling this with other PositionComponent writers...
}

// ... or used to build a WorldView that has proper asserts
WorldView wv = d.create_world_view(&world);
```

Scheduler

```cpp

Scheduler::Builder sb;

// Nodes can be created that will consume a WorldView*.
Scheduler::Node movement_node =
    sb.add_node()
      .with_decl(MovementSystem::world_view_decl())
      .main_thread_only() // any_thread by default
      .build([](WorldView* wv) {
        MovementSystem::update(wv);
        return emptyPromise();
      });
// Example scheduling (usually done in the Scheduler context)
auto movement_task_promise = movement_node->schedule(&world);

// Nodes can also do async tasks
Scheduler::Node animation_system =
    sb.add_node()
      .with_decl(AssembleAnimationDataSystem::world_view_decl())
      .depends_on(movement_node) // This will not be executed
      .build([any_thread_task_list](WorldView* wv) {
        auto assemble_promise = AssembleAnimationDataSystem::run(wv);
        return assemble_promise->then_return_empty(any_thread_task_list);
      });

// Scheduler parameters can be added...
sb.max_spin_time(std::chrono::milliseconds(2));

// A scheduler can be built that will execute all nodes. On builds with
//  IG_ENABLE_ECS_VALIDATION enabled, this will assert if a call graph
//  cannot be built.
Scheduler update_scheduler = sb.build();

// And that scheduler can be executed at the appropriate time
update_scheduler.execute(main_thread_task_list, any_thread_task_list, &world);
```
