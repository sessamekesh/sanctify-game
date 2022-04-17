# C++ Style Guide for the Sanctify Game project

## General Guideline

None of these rules are absolute.

Strive for readability and consistency. You should assume that the next person to read your code will
be both a total idiot, _and_ in charge of making a judgement call of how good your code is.

_That's not really far off from the truth, I have to read all the code written in this code base :-)_

## Naming

* Class names are `UpperCamelCase`
* Class members are private, and named `lower_snake_case_` with a trailing underscore (`_`) character
* Class method names are `lower_snake_case` named
  * Static builder methods are named `UpperCamelCase`, e.g. `FooClass::Create()`
* Struct data members are named `lowerCamelCase`
* Respect `class`/`struct` idioms
  * `class` keyword should be reserved for types with no public data members
  * `struct` keyword should be reserved for types with only public data members and no non-static methods
    * Equality/copy/move operators is OK
  * The compiler doesn't care which keyword you use. I do, for readability.

## ECS

### indigo::igecs::WorldView

indigo::igecs::WorldView is a thin wrapper around an entt::registry* that has
the additional property of an optional runtime data access assertion layer
(controlled by the CMake argument "IG_ENABLE_ECS_VALIDATION").

* For any code where the context isn't known (e.g. common libraries, utilities) 
  prefer indigo::igecs::WorldView
  * Direct accesses to entt::registry are acceptable from the main thread in top-level
    application code, but frowned upon - use indigo::igecs::WorldView::Thin to the same effect
  * Systems should expose a "decl" method that provides an indigo::igecs::WorldView::Decl that
    can be used by scheduling nodes to construct a WorldView with the correct assertions.
  * System "decl" method should also cover anything that may be covered by downstream utilities
  * Unit tests should be exhaustive to all branches with downstream util calls to attempt to
    catch any bad introductions of new read/writes to/from world.
* For loading, initialization, and pre-tick/post-tick logic, using a thin WorldView (no validation)
  is fine. However, if at all possible, _only_ write from the main thread, and schedule side thread
  reads after all writes will have finished.
* For tick logic, using a igecs::Scheduler is a good way to handle concurrency.
  * Be careful to set all component read/writes
  * Be careful to build out the proper dependency ordering.
  * TODO (sessamekesh): validate this claim by actually using the thing

### Component Types

* Components should generally be structs with no logic attached to them
* Components should end with `Component`, e.g. `PositionComponent`
* Context components should be prefixed with `Ctx`, e.g. `CtxTimeElapsedComponent`

### Systems and Utilities

Most mutations should happen in either a _system_ or _utility_.

Both systems and utilities are classes with no data members and only static methods - they serve
as a namespace of sorts. This isn't necessarily the only (or maybe not even the best) structure,
but it's one that's workable.

General guidelines:
* Iterating over a component view should generally be done in systems
  * If this must be done in a utility, try to at least hint at iteration in the name - e.g. `draw_all_debug_renderables`
* Utility methods should be very thin, and operate only on the minimum amount of data necessary.
* Context variable access should happen through utilities where possible, to allow default construction behavior
* If a component type can be hidden, prefer keeping it in an anonymous namespace in the system/util implementation file.