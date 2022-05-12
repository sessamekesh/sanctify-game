#include "stage_client_message.h"

#include <igcore/log.h>
#include <sanctify/pve/net_logic_common/proto/client_messages.pb.h>

using namespace sanctify;
using namespace pve;
using namespace indigo;
using namespace core;

const indigo::igecs::WorldView::Decl& StageClientMessageSystem::decl() {
  static indigo::igecs::WorldView::Decl decl =
      indigo::igecs::WorldView::Decl()
          .evt_consumes<proto::PveClientSingleMessage>();

  return decl;
}

void StageClientMessageSystem::run(indigo::igecs::WorldView* wv) {
  // TODO (sessamekesh): Actually stage these in an intermediate area for
  //  sending to the server - or, alternatively, actually send them to
  //  the server!
  Vector<proto::PveClientSingleMessage> messages =
      wv->consume_events<proto::PveClientSingleMessage>();

  for (int i = 0; i < messages.size(); i++) {
    if (messages[i].has_player_movement_request()) {
      Logger::log("StageClientMessageSystem")
          << "Movement request: "
          << messages[i].player_movement_request().position().x() << ", "
          << messages[i].player_movement_request().position().y();
    }
  }
}
