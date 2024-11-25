#include "mediator.hpp"
#include "debug.hpp"
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

ReplMediator::ReplMediator(Ref<TCPServer> server, Callable reply) :
  server(server), reply(reply) {
  node_registry = std::make_shared<ReplNodeRegistry>();
}

struct ReplMessageHandler {
  ReplMediator* mediator;

  void operator()(const ReplMessage::PublishNode &m) {
    mediator->node_registry->register_node(m.node_id, std::move(m.node_name));
  }
  void operator()(const ReplMessage::UnpublishNode &m) {
    mediator->node_registry->unregister_node(m.node_id);
  }
  void operator()(const ReplMessage::EvalResponse &m) {
  }
};

bool ReplMediator::mediate(MessageQueue &queue) {
  int interactions = 0;
  auto message = std::move(queue.pop());
  if (message) {
    std::visit(ReplMessageHandler{this}, message->payload);
    interactions++;
  }

  if (server->is_connection_available()) {
    // TODO: client starts with most recent Scheme node
    auto connection = ReplConnection(server->take_connection(), node_registry);
    gd::print("Scheme repl client connected.");
    connection.send_prompt();
    connections.emplace_back(std::move(connection));
    interactions++;
  }

  for (auto connection = connections.begin(); connection != connections.end();) {
    auto status = connection->process_with(request_compiler, reply);
    if (status == ReplConnection::DISCONNECTED) {
      connection = connections.erase(connection);
      gd::print("Scheme repl client disconnected.");
    } else {
      connection++;
      interactions += (status != ReplConnection::IDLE) ? 1 : 0;
    }
  }
  return interactions > 0;
}
