#ifndef LIBS_IGECS_INCLUDE_IGECS_WORLD_VIEW_H
#define LIBS_IGECS_INCLUDE_IGECS_WORLD_VIEW_H

#include <igcore/config.h>
#include <igcore/pod_vector.h>
#include <igecs/ctti_type_id.h>

#ifdef IG_ENABLE_ECS_VALIDATION
#include <igcore/log.h>
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

    WorldView create(entt::registry* registry);

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
#endif
    return true;
  }

 public:
  WorldView(entt::registry* registry, Decl decl);

#ifdef IG_ECS_TEST_VALIDATIONS
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
#endif

  template <typename T>
  T& mut_ctx() {
#ifdef IG_ENABLE_ECS_VALIDATION
    ::assert_and_print<T>(decl_.can_ctx_write<T>(), "mut_ctx");
#endif
    return registry_->ctx<T>();
  }

  template <typename T>
  const T& ctx() {
#ifdef IG_ENABLE_ECS_VALIDATION
    ::assert_and_print<T>(decl_.can_ctx_read<T>(), "ctx");
#endif
    return registry_->ctx<T>();
  }

  template <typename T>
  bool has(entt::entity e) {
#ifdef IG_ENABLE_ECS_VALIDATION
    ::assert_and_print<T>(decl_.can_read<T>(), "has");
#endif
    return registry_->try_get<T>(e) != nullptr;
  }

  template <typename T>
  T& write(entt::entity e) {
#ifdef IG_ENABLE_ECS_VALIDATION
    ::assert_and_print<T>(decl_.can_write<T>(), "write");
#endif
    return registry_->get<T>(e);
  }

  template <typename ComponentT, typename... Args>
  ComponentT& attach(entt::entity e, Args&&... args) {
#ifdef IG_ENABLE_ECS_VALIDATION
    ::assert_and_print<ComponentT>(decl_.can_write<ComponentT>(), "attach");
#endif
    return registry_->emplace<ComponentT, Args...>(e,
                                                   std::forward<Args>(args)...);
  }

  template <typename T>
  const T& read(entt::entity e) {
#ifdef IG_ENABLE_ECS_VALIDATION
    ::assert_and_print<T>(decl_.can_read<T>(), "read");
#endif
    return registry_->get<T>(e);
  }

  template <typename T>
  size_t remove(entt::entity e) {
#ifdef IG_ENABLE_ECS_VALIDATION
    ::assert_and_print<T>(decl_.can_write<T>(), "remove");
#endif
    return registry_->remove<T>(e);
  }

  template <typename T>
  void remove_ctx() {
#ifdef IG_ENABLE_ECS_VALIDATION
    ::assert_and_print<T>(decl_.can_ctx_write<T>(), "remove_ctx");
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
