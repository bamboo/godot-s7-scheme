#ifndef GODOT_S7_SCHEME_REPL_REQUEST_MEDIATOR_HPP
#define GODOT_S7_SCHEME_REPL_REQUEST_MEDIATOR_HPP

#include "connection.hpp"
#include "message.hpp"
#include "node_registry.hpp"
#include "request_compiler.hpp"
#include "thread_safe_queue.hpp"
#include <godot_cpp/classes/tcp_server.hpp>
#include <vector>

class ReplMediator {
public:
  using MessageQueue = ThreadSafeQueue<ReplMessage>;

  ReplMediator(godot::Ref<godot::TCPServer> server, godot::Callable reply);

public:
  /**
   * Process all pending interactions.
   * @return true when at least one interaction was processed, false otherwise.
   */
  bool mediate(MessageQueue& queue);

private:
  friend struct ReplMessageHandler;

private:
  godot::Ref<godot::TCPServer> server;
  godot::Callable reply;
  std::shared_ptr<ReplNodeRegistry> node_registry;
  std::vector<ReplConnection> connections;
  ReplRequestCompiler request_compiler;
};
#endif
