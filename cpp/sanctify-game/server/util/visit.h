#ifndef SANCTIFY_GAME_SERVER_UTIL_VISIT_H
#define SANCTIFY_GAME_SERVER_UTIL_VISIT_H

#include <functional>
#include <variant>

namespace sanctify {

struct Visit {
  template <typename T, typename VariantT>
  static void visit_if_t(VariantT& variant, std::function<void(T&)> cb) {
    if (std::holds_alternative<T>(variant)) {
      T& val = std::get<T>(variant);
      cb(val);
    }
  }

  template <typename T, typename VariantT>
  static void visit_if_t(const VariantT& variant,
                         std::function<void(const T&)> cb) {
    if (std::holds_alternative<T>(variant)) {
      const T& val = std::get<T>(variant);
      cb(val);
    }
  }
};

}  // namespace sanctify

#endif
