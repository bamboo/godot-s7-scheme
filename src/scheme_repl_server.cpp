#include "scheme_repl_server.hpp"
#include "repl/mediator.hpp"
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;
using gd = UtilityFunctions;

void SchemeReplServer::publish_node(const Scheme *node) {
  if (thread.is_null()) {
    return;
  }
  auto node_name = node->get_path().slice(-2).get_concatenated_names();
  message_queue.push(std::move(ReplMessage::publish_node(std::move(node_name), node->get_instance_id())));
}

void SchemeReplServer::unpublish_node(const Scheme *node) {
  if (thread.is_null()) {
    return;
  }
  message_queue.push(std::move(ReplMessage::unpublish_node(node->get_instance_id())));
}

void SchemeReplServer::reply(Variant result, uint64_t request_id) {
  if (thread.is_null()) {
    return;
  }
  message_queue.push(std::move(ReplMessage::eval_response(request_id, std::move(result))));
}

void SchemeReplServer::server_loop(int tcp_port, const String &tcp_bind_address) {
  Ref<TCPServer> tcp_server;
  tcp_server.instantiate();

  auto error = tcp_server->listen(tcp_port, tcp_bind_address);
  ERR_FAIL_COND_MSG(
      error != OK,
      ("Failed to start scheme repl server: " + gd::error_string(error)));

  gd::print("Scheme repl server listening on local port ", tcp_server->get_local_port());

  auto mediator = ReplMediator(tcp_server, Callable::create(this, "reply"));
  while (!exit_thread) {
    if (!mediator.mediate(message_queue)) {
      OS::get_singleton()->delay_msec(50);
    }
  }
  tcp_server->stop();
}

std::optional<std::pair<uint16_t, String>> parse_repl_args() {
  // TODO: accept --s7-tcp-address=<address>
  String tcp_bind_address = "127.0.0.1";

  auto args = OS::get_singleton()->get_cmdline_args();
  for (auto arg = args.begin(); arg != args.end(); ++arg) {
    if (arg->begins_with("--s7-tcp-port")) {
      auto parts = arg->split("=");
      auto tcp_port = parts.size() > 1 ? parts[1].to_int() : 0;
      return std::make_pair(tcp_port, tcp_bind_address);
    }
  }
  return std::nullopt;
}

Error SchemeReplServer::init() {
  ERR_FAIL_COND_V_MSG(thread.is_valid(), ERR_BUG, "Scheme repl server can only be started once!");

  auto repl_args = parse_repl_args();
  if (!repl_args) {
    return Error::OK;
  }

  int tcp_port = repl_args->first;
  const String &tcp_bind_address = repl_args->second;

  exit_thread = false;
  thread.instantiate();
  return thread->start(
      Callable::create(this, "server_loop").bind(tcp_port, tcp_bind_address),
      Thread::PRIORITY_LOW);
}

void SchemeReplServer::finish() {
  if (thread.is_null()) {
    return;
  }

  exit_thread = true;
  thread->wait_to_finish();

  thread.unref();
}

SchemeReplServer *SchemeReplServer::singleton = NULL;

SchemeReplServer *SchemeReplServer::get_singleton() {
  return singleton;
}

SchemeReplServer::SchemeReplServer() {
  singleton = this;
}

void SchemeReplServer::_bind_methods() {
  ClassDB::bind_method(D_METHOD("server_loop"), &SchemeReplServer::server_loop);
  ClassDB::bind_method(D_METHOD("reply"), &SchemeReplServer::reply);
}
