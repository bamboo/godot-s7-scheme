#ifndef GODOT_S7_SCHEME_REPL_REQUEST_CONNECTION_HPP
#define GODOT_S7_SCHEME_REPL_REQUEST_CONNECTION_HPP

#include "request_compiler.hpp"
#include "node_registry.hpp"
#include <godot_cpp/classes/stream_peer_tcp.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <memory>

class ReplConnection {
public:
  enum Status { IDLE, TRANSMITTING, DISCONNECTED };
  ReplConnection(godot::Ref<godot::StreamPeerTCP> tcp_stream, std::shared_ptr<ReplNodeRegistry> node_registry) :
    tcp_stream(tcp_stream), node_registry(node_registry), target_node(node_registry->get_most_recent()) {}

public:
  void send_prompt();
  Status process_with(ReplRequestCompiler &compiler, const godot::Callable& reply);
  void disconnect();

private:
  godot::String get_prompt();
  void send(const godot::CharString &s);
  void send(const char *p, size_t count);
  void send(char c);
  bool process_buffer_with(ReplRequestCompiler &compiler, const godot::Callable &reply);
  void process_eval_request_with(ReplRequestCompiler &compiler, const godot::Callable &reply);
  void send_available_nodes();

private:
  godot::Ref<godot::StreamPeerTCP> tcp_stream;
  godot::PackedByteArray buffer;
  std::shared_ptr<ReplNodeRegistry> node_registry;
  std::optional<ReplNodeRegistry::NodeRecord> target_node;
};
#endif
