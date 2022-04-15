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

void WorldView::Decl::merge_in_decl(const WorldView::Decl& o) {
  for (int i = 0; i < o.reads_.size(); i++) {
    if (!reads_.contains(o.reads_[i])) {
      reads_.push_back(o.reads_[i]);
    }
  }

  for (int i = 0; i < o.writes_.size(); i++) {
    if (!writes_.contains(o.writes_[i])) {
      writes_.push_back(o.writes_[i]);
    }
  }

  for (int i = 0; i < o.ctx_reads_.size(); i++) {
    if (!ctx_reads_.contains(o.ctx_reads_[i])) {
      ctx_reads_.push_back(o.ctx_reads_[i]);
    }
  }

  for (int i = 0; i < o.ctx_writes_.size(); i++) {
    if (!ctx_writes_.contains(o.ctx_writes_[i])) {
      ctx_writes_.push_back(o.ctx_writes_[i]);
    }
  }
}
