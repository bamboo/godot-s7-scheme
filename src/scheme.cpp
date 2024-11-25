#include "scheme.hpp"
#include "ffi.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include "scheme_repl_server.hpp"

using namespace godot;

Scheme::Scheme() {
  define_variant_ffi(scheme);
  auto node = make_variant_object(scheme.get(), this);
  scheme.define_constant_with_documentation("*node*", node, "this Godot node");
}

Scheme::~Scheme() { _process_symbol = nullptr; }

void Scheme::_ready() {
  load_prelude();
  load_script();
}

void Scheme::_process(double delta) {
  if (_process_symbol) {
    scheme.call(_process_symbol.get(), delta);
  }
}
void Scheme::_enter_tree() {
  // TODO: move initialization here
  SchemeReplServer::get_singleton()->publish_node(this);
  Node::_enter_tree();
}

void Scheme::_exit_tree() {
  SchemeReplServer::get_singleton()->unpublish_node(this);

  if (_process_symbol) {
    auto _ = scheme.call_optional("_exit_tree");
  }
  Node::_exit_tree();
}

void Scheme::define(
    const godot::String &name,
    const godot::Variant &value,
    const String &help) const {
  scheme.define(name.utf8(), variant_to_scheme(scheme.get(), value), help.utf8());
}

void Scheme::set_scheme_script(const Ref<godot::SchemeScript> &p_scheme_script) {
  scheme_script = p_scheme_script;
  if (is_node_ready()) {
    load_script();
  }
}

void Scheme::load_prelude() {
  for (int i = 0; i < prelude.size(); ++i) {
    auto script = Object::cast_to<SchemeScript>(prelude[i]);
    DEV_ASSERT(script != nullptr);
    load(script);
  }
}

void Scheme::load_script() {
  if (scheme_script.is_null()) {
    _process_symbol = nullptr;
    return;
  }

  load(scheme_script.ptr());
  _process_symbol = scheme.make_symbol("_process");
}

void Scheme::load(const godot::SchemeScript *script) const {
  load_string(script->get_code());
}

void Scheme::load_string(const String &code) const {
  scheme.load_string(code);
}

Variant Scheme::eval(const String &code) {
  return scheme_to_variant(scheme.get(), scheme.eval(code));
}

void Scheme::eval_async(const String &code, const Callable &continuation) {
  auto result = eval(code);
  continuation.call_deferred(result);
}

s7_pointer array_to_list(s7_scheme *sc, const Array &array) {
  auto list = s7_nil(sc);
  auto arg_count = static_cast<int>(array.size());
  while (arg_count > 0) {
    auto arg = variant_to_scheme(sc, array[--arg_count]);
    list = s7_cons(sc, arg, list);
  }
  return list;
}

Variant Scheme::apply(const String &symbol, const Array &args) const {
  auto sc = scheme.get();
  auto func = s7_name_to_value(sc, symbol.utf8().ptr());
  auto scheme_args = array_to_list(sc, args);
  return scheme_to_variant(sc,
      s7_call_with_location(sc, func, scheme_args, __func__, __FILE__, __LINE__));
}

void Scheme::_bind_methods() {
  ClassDB::bind_method(D_METHOD("set_prelude", "p_prelude"), &Scheme::set_prelude);
  ClassDB::bind_method(D_METHOD("get_prelude"), &Scheme::get_prelude);
  ClassDB::add_property("Scheme",
      PropertyInfo(Variant::ARRAY,
          "prelude",
          PROPERTY_HINT_TYPE_STRING,
          vformat("%d/%d:SchemeScript", Variant::OBJECT, PROPERTY_HINT_RESOURCE_TYPE)),
      "set_prelude",
      "get_prelude");

  ClassDB::bind_method(
      D_METHOD("set_scheme_script", "p_scheme_script"),
      &Scheme::set_scheme_script);
  ClassDB::bind_method(D_METHOD("get_scheme_script"), &Scheme::get_scheme_script);
  ClassDB::add_property("Scheme",
      PropertyInfo(
          Variant::OBJECT,
          "scheme_script",
          PROPERTY_HINT_RESOURCE_TYPE,
          "SchemeScript"),
      "set_scheme_script",
      "get_scheme_script");

  ClassDB::bind_method(
      D_METHOD("define", "p_name", "p_value", "p_help"),
      &Scheme::define,
      DEFVAL(""));
  ClassDB::bind_method(D_METHOD("eval", "p_code"), &Scheme::eval);
  ClassDB::bind_method(D_METHOD("eval_async", "p_code", "p_continuation"), &Scheme::eval_async);
  ClassDB::bind_method(D_METHOD("apply", "p_symbol", "p_args"),
      &Scheme::apply,
      DEFVAL(Array()));
  ClassDB::bind_method(D_METHOD("load", "p_scheme_script"), &Scheme::load);
  ClassDB::bind_method(D_METHOD("load_string", "p_code"), &Scheme::load_string);
}
