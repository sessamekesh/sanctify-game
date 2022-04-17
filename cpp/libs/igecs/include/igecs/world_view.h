#ifndef LIBS_IGECS_INCLUDE_IGECS_WORLD_VIEW_H
#define LIBS_IGECS_INCLUDE_IGECS_WORLD_VIEW_H

#include <igcore/config.h>
#include <igcore/pod_vector.h>
#include <igecs/ctti_type_id.h>

#ifdef IG_ENABLE_ECS_VALIDATION
#include <igcore/log.h>

#include <set>
#endif

#include <entt/entt.hpp>

namespace {
template <typename T>
std::string failmsg(const char* method) {
  return (std::string("ECS validation failure: method ") + method +
          std::string(" failed for type ") +
          indigo::igecs::CttiTypeId::name<T>());
}

template <typename T>
void assert_and_print(bool condition, const char* method) {
  if (!condition) {
    std::cerr << ::failmsg<T>(method) << std::endl;
  }
  assert(condition);
}

}  // namespace

namespace indigo::igecs {
class WorldView {
 public:
  class Decl {
   public:
    Decl();
    Decl(const Decl&) = default;
    Decl(Decl&&) = default;
    Decl& operator=(const Decl&) = default;
    Decl& operator=(Decl&&) = default;

    static Decl Thin();

    void merge_in_decl(const Decl& o);

    template <typename T>
    Decl& reads() {
#ifdef IG_ENABLE_ECS_VALIDATION
      reads_.push_back(CttiTypeId::of<std::remove_const_t<T>>());
#endif
      return *this;
    }

    template <typename T>
    Decl& writes() {
#ifdef IG_ENABLE_ECS_VALIDATION
      writes_.push_back(CttiTypeId::of<std::remove_const_t<T>>());
      reads_.push_back(CttiTypeId::of<std::remove_const_t<T>>());
#endif
      return *this;
    }

    template <typename T>
    Decl& ctx_reads() {
#ifdef IG_ENABLE_ECS_VALIDATION
      ctx_reads_.push_back(CttiTypeId::of<std::remove_const_t<T>>());
#endif
      return *this;
    }

    template <typename T>
    Decl& ctx_writes() {
#ifdef IG_ENABLE_ECS_VALIDATION
      ctx_reads_.push_back(CttiTypeId::of<std::remove_const_t<T>>());
      ctx_writes_.push_back(CttiTypeId::of<std::remove_const_t<T>>());
#endif
      return *this;
    }

    template <typename T>
    [[nodiscard]] bool can_read() const {
#ifdef IG_ENABLE_ECS_VALIDATION
      return allow_all_ ||
             reads_.contains(CttiTypeId::of<std::remove_const_t<T>>());
#else
      return true;
#endif
    }

    template <typename T>
    [[nodiscard]] bool can_write() const {
#ifdef IG_ENABLE_ECS_VALIDATION
      return allow_all_ ||
             writes_.contains(CttiTypeId::of<std::remove_const_t<T>>());
#else
      return true;
#endif
    }

    template <typename T>
    [[nodiscard]] bool can_ctx_read() const {
#ifdef IG_ENABLE_ECS_VALIDATION
      return allow_all_ ||
             ctx_reads_.contains(CttiTypeId::of<std::remove_const_t<T>>());
#else
      return true;
#endif
    }

    template <typename T>
    [[nodiscard]] bool can_ctx_write() const {
#ifdef IG_ENABLE_ECS_VALIDATION
      return allow_all_ ||
             ctx_writes_.contains(CttiTypeId::of<std::remove_const_t<T>>());
#else
      return true;
#endif
    }

    WorldView create(entt::registry* registry) const;

    const core::PodVector<CttiTypeId>& list_reads() const { return reads_; }
    const core::PodVector<CttiTypeId>& list_writes() const { return writes_; }
    const core::PodVector<CttiTypeId>& list_ctx_reads() const {
      return ctx_reads_;
    }
    const core::PodVector<CttiTypeId>& list_ctx_writes() const {
      return ctx_writes_;
    }

   private:
    Decl(bool allow_all);

    bool allow_all_;
#ifdef IG_ENABLE_ECS_VALIDATION
    indigo::core::PodVector<CttiTypeId> reads_;
    indigo::core::PodVector<CttiTypeId> writes_;
    indigo::core::PodVector<CttiTypeId> ctx_reads_;
    indigo::core::PodVector<CttiTypeId> ctx_writes_;
#endif
  };

 private:
  // Base case
  template <int = 0>
  bool view_test() const {
    return true;
  }

  template <
      typename T, typename... Others,
      typename std::enable_if<std::is_const<T>::value, int>::type* = nullptr>
  bool view_test() const {
#ifdef IG_ENABLE_ECS_VALIDATION
    if (!decl_.can_read<T>()) {
      indigo::core::Logger::err("WorldView")
          << "MUTABLE view_test failed for type " << CttiTypeId::name<T>();
      return false;
    }
    return view_test<Others...>();
    read_types_.insert(CttiTypeId::of<T>());
#endif
    return true;
  }

  template <
      typename T, typename... Others,
      typename std::enable_if<!std::is_const<T>::value, int>::type* = nullptr>
  bool view_test() const {
#ifdef IG_ENABLE_ECS_VALIDATION
    if (!decl_.can_write<T>()) {
      indigo::core::Logger::err("WorldView")
          << "IMMUTABLE view_test failed for type " << CttiTypeId::name<T>();
      return false;
    }
    return view_test<Others...>();
    write_types_.insert(CttiTypeId::of<T>());
#endif
    return true;
  }

 public:
  WorldView(entt::registry* registry, Decl decl);
  static WorldView Thin(entt::registry* registry);

#ifdef IG_ENABLE_ECS_VALIDATION
  template <typename T>
  bool can_read() {
    return decl_.can_read<T>();
  }

  template <typename T>
  bool can_write() {
    return decl_.can_write<T>();
  }

  template <typename T>
  bool can_ctx_read() {
    return decl_.can_ctx_read<T>();
  }

  template <typename T>
  bool can_ctx_write() {
    return decl_.can_ctx_write<T>();
  }

 private:
  mutable std::set<CttiTypeId> read_types_;
  mutable std::set<CttiTypeId> write_types_;
  mutable std::set<CttiTypeId> ctx_read_types_;
  mutable std::set<CttiTypeId> ctx_write_types_;

 public:
  template <typename T>
  bool has_read() const {
    return read_types_.count(CttiTypeId::of<T>()) > 0;
  }
  template <typename T>
  bool has_written() const {
    return write_types_.count(CttiTypeId::of<T>()) > 0;
  }
  template <typename T>
  bool has_ctx_read() const {
    return ctx_read_types_.count(CttiTypeId::of<T>()) > 0;
  }
  template <typename T>
  bool has_ctx_written() const {
    return ctx_write_types_.count(CttiTypeId::of<T>()) > 0;
  }
#endif

  template <typename T>
  T& mut_ctx() {
#ifdef IG_ENABLE_ECS_VALIDATION
    ::assert_and_print<T>(decl_.can_ctx_write<T>(), "mut_ctx");
    ctx_write_types_.insert(CttiTypeId::of<T>());
#endif
    return registry_->ctx<T>();
  }

  template <typename T>
  const T& ctx() {
#ifdef IG_ENABLE_ECS_VALIDATION
    ::assert_and_print<T>(decl_.can_ctx_read<T>(), "ctx");
    ctx_read_types_.insert(CttiTypeId::of<T>());
#endif
    return registry_->ctx<T>();
  }

  template <typename T, typename... Args>
  T& mut_ctx_or_set(Args&&... args) {
#ifdef IG_ENABLE_ECS_VALIDATION
    ::assert_and_print<T>(decl_.can_ctx_write<T>(), "mut_ctx_or_set");
    ctx_write_types_.insert(CttiTypeId::of<T>());
#endif
    return registry_->ctx_or_set<T, Args...>(std::forward<Args>(args)...);
  }

  template <typename T>
  bool has(entt::entity e) {
#ifdef IG_ENABLE_ECS_VALIDATION
    ::assert_and_print<T>(decl_.can_read<T>(), "has");
    read_types_.insert(CttiTypeId::of<T>());
#endif
    return registry_->try_get<T>(e) != nullptr;
  }

  template <typename T>
  bool ctx_has() const {
#ifdef IG_ENABLE_ECS_VALIDATION
    ::assert_and_print<T>(decl_.can_ctx_read<T>(), "ctx_has");
    ctx_read_types_.insert(CttiTypeId::of<T>());
#endif
    return registry_->try_ctx<T>() != nullptr;
  }

  template <typename T>
  T& write(entt::entity e) {
#ifdef IG_ENABLE_ECS_VALIDATION
    ::assert_and_print<T>(decl_.can_write<T>(), "write");
    write_types_.insert(CttiTypeId::of<T>());
#endif
    return registry_->get<T>(e);
  }

  template <typename ComponentT, typename... Args>
  ComponentT& attach(entt::entity e, Args&&... args) {
#ifdef IG_ENABLE_ECS_VALIDATION
    ::assert_and_print<ComponentT>(decl_.can_write<ComponentT>(), "attach");
    write_types_.insert(CttiTypeId::of<ComponentT>());
#endif
    return registry_->emplace<ComponentT, Args...>(e,
                                                   std::forward<Args>(args)...);
  }

  template <typename ComponentT, typename... Args>
  ComponentT& attach_ctx(Args&&... args) {
#ifdef IG_ENABLE_ECS_VALIDATION
    ::assert_and_print<ComponentT>(decl_.can_ctx_write<ComponentT>(),
                                   "attach_ctx");
    ctx_write_types_.insert(CttiTypeId::of<ComponentT>());
#endif
    return registry_->set<ComponentT, Args...>(std::forward<Args>(args)...);
  }

  template <typename T>
  const T& read(entt::entity e) {
#ifdef IG_ENABLE_ECS_VALIDATION
    ::assert_and_print<T>(decl_.can_read<T>(), "read");
    read_types_.insert(CttiTypeId::of<T>());
#endif
    return registry_->get<T>(e);
  }

  template <typename T>
  size_t remove(entt::entity e) {
#ifdef IG_ENABLE_ECS_VALIDATION
    ::assert_and_print<T>(decl_.can_write<T>(), "remove");
    write_types_.insert(CttiTypeId::of<T>());
#endif
    return registry_->remove<T>(e);
  }

  template <typename T>
  void remove_ctx() {
#ifdef IG_ENABLE_ECS_VALIDATION
    ::assert_and_print<T>(decl_.can_ctx_write<T>(), "remove_ctx");
    ctx_write_types_.insert(CttiTypeId::of<T>());
#endif
    registry_->unset<T>();
  }

  template <typename Component, typename... Other, typename... Exclude>
  [[nodiscard]] entt::basic_view<entt::registry::entity_type,
                                 entt::get_t<Component, Other...>,
                                 entt::exclude_t<Exclude...>>
  view(entt::exclude_t<Exclude...> e = {}) {
#ifdef IG_ENABLE_ECS_VALIDATION
    bool rsl = view_test<Component, Other...>();
    assert(rsl);
#endif
    return registry_->view<Component, Other..., Exclude...>(e);
  }

  inline entt::entity create() { return registry_->create(); }

 private:
  entt::registry* registry_;
  Decl decl_;
};
}  // namespace indigo::igecs

#endif
