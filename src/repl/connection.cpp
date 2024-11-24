#include "connection.hpp"
#include "debug.hpp"

using namespace godot;

void ReplConnection::disconnect() {
  tcp_stream->disconnect_from_host();
}

String ReplConnection::get_prompt() {
  // auto owner = (target_scheme != nullptr ? (Node *)target_scheme : this)->get_owner();
  // auto path = owner != nullptr ? "" + owner->get_name() + "/" + get_name() : "" + get_name();
  //return "\ns7@(" + path + ")> ";
  return "\ns7@(:)> ";
}

void ReplConnection::send(char c) {
  tcp_stream->put_8(c);
}

void ReplConnection::send(const char *p, size_t count) {
  for (int i = 0; i < count; ++i) {
    send(p[i]);
  }
}

void ReplConnection::send(const CharString &s) {
  send(s.get_data(), s.length());
}

void ReplConnection::send_prompt() {
  send(get_prompt().utf8());
}

ReplConnection::Status ReplConnection::process_with(ReplRequestCompiler &compiler) {
  if (tcp_stream->get_status() != StreamPeerTCP::STATUS_CONNECTED) {
    return ReplConnection::DISCONNECTED;
  }

  auto available = tcp_stream->get_available_bytes();
  auto originally_available = available;
  while (available > 0) {
    available--;
    auto ch = tcp_stream->get_8();

#if DEBUG_REPL_INTERACTIONS
    putchar(ch);
#endif

    if (ch == '\n' && available == 0) {
      if (!process_buffer_with(compiler)) {
        return ReplConnection::DISCONNECTED;
      }
    } else {
      buffer.push_back(ch);
    }
  }
  if (tcp_stream->poll() != Error::OK) {
    return ReplConnection::DISCONNECTED;
  }
  return originally_available > 0
      ? ReplConnection::TRANSMITTING
      : ReplConnection::IDLE;
}

bool ReplConnection::process_buffer_with(ReplRequestCompiler &compiler) {
  if (buffer.size() == 2 && buffer.get_string_from_utf8() == ",q") {
    // disconnection from repl
    return false;
  }
  if (buffer.size() == 3 && buffer.get_string_from_utf8() == ",ls") {
    buffer.clear();
    send_available_nodes();
    return true;
  }

  process_eval_request_with(compiler);
  return true;
}

void ReplConnection::send_available_nodes() {
  auto node_paths = node_registry->get_available_node_names();
  for (auto path = node_paths.begin(); path != node_paths.end(); ++path) {
    send(path->utf8());
    send('\n');
  }
  send_prompt();
}

void ReplConnection::process_eval_request_with(ReplRequestCompiler &compiler) {
  auto result = compiler.eval(buffer);
  buffer.clear();

  if (result.first) {
    gd::printerr(result.first);
    send(result.first);
    send('\n');
  }

  DEBUG_REPL(result.second);
  send(result.second);
  send('\n');

  send_prompt();
}
