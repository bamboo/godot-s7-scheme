#ifndef GODOT_S7_SCHEME_REPL_MESSAGE_HPP
#define GODOT_S7_SCHEME_REPL_MESSAGE_HPP

#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <variant>

struct ReplMessage {

  static ReplMessage publish_node(godot::StringName node_name, uint64_t node_id) {
    return ReplMessage{ PublishNode{ node_name, node_id } };
  }

  static ReplMessage unpublish_node(uint64_t node_id) {
    return ReplMessage{ UnpublishNode{ node_id } };
  }

  static ReplMessage eval_response(uint64_t request_id, godot::Variant result) {
    return ReplMessage{ EvalResponse{ request_id, result } };
  }

  struct PublishNode {
    godot::StringName node_name;
    uint64_t node_id;
  };

  struct UnpublishNode {
    uint64_t node_id;
  };

  struct EvalResponse {
    uint64_t request_id;
    godot::Variant result;
  };

  using Payload = std::variant<PublishNode, UnpublishNode, EvalResponse>;
  Payload payload;
};

#endif
