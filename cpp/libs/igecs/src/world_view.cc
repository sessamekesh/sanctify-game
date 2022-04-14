#include <igecs/ctti_type_id.h>
#include <igecs/world_view.h>

using namespace indigo;
using namespace igecs;

WorldView::Decl WorldView::Decl::Thin() { return Decl(true); }

WorldView::Decl::Decl() : allow_all_(false) {}

WorldView::Decl::Decl(bool allow_all) : allow_all_(allow_all) {}

WorldView WorldView::Decl::create(entt::registry* registry) {
  return WorldView(registry, *this);
}

WorldView::WorldView(entt::registry* registry, Decl decl)
    : registry_(registry), decl_(std::move(decl)) {
  assert(registry != nullptr);
}
