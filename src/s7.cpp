
#include "s7.hpp"
#include "debug_macros.hpp"
#include <godot_cpp/variant/utility_functions.hpp>
#include <vector>

using namespace godot;

class s7_scheme_context {
public:
  void print_error(uint8_t char_code) {
    if (char_code == '\n') {
      if (!error_buffer.empty()) {
        auto buffer = reinterpret_cast<const char *>(error_buffer.data());
        UtilityFunctions::printerr(
            String::utf8(buffer, static_cast<int>(error_buffer.size())));
        error_buffer.clear();
      }
    } else {
      error_buffer.push_back(char_code);
    }
  }

private:
  std::vector<uint8_t> error_buffer;
};

void add_scheme_mapping(s7_scheme *sc, s7_scheme_context *scheme) {
  s7_define_constant(sc, "*ctx*", s7_make_c_pointer(sc, scheme));
}

s7_scheme_context *context_of(s7_scheme *sc) {
  return static_cast<s7_scheme_context *>(s7_c_pointer(s7_name_to_value(sc, "*ctx*")));
}

void godot_print_error(s7_scheme *sc, uint8_t c, s7_pointer _port) {
  context_of(sc)->print_error(c);
}

void s7::set_current_error_port_function(s7_output_port_function_t f) const {
  auto sc = scheme.get();
  auto port = s7_open_output_function(sc, f);
  s7_set_current_error_port(sc, port);
}

s7::s7() {
  scheme = std::shared_ptr<s7_scheme>(s7_init(), s7_free);
  scheme_context = std::make_shared<s7_scheme_context>();

  set_current_error_port_function(godot_print_error);

  add_scheme_mapping(scheme.get(), scheme_context.get());
}

void s7::load_string(const String &str) const {
  auto sc = get();
  auto code = str.utf8();
  auto res = s7_load_c_string(sc, code, code.length());
  WATCH(res);
}

s7_pointer s7::eval(const String &code) const {
  auto sc = get();
  auto str = code.utf8();
  return s7_eval_c_string(sc, str);
}

s7_pointer s7::define(const char *name, s7_pointer value, const char *help) const {
  auto sc = get();
  return s7_define_variable_with_documentation(sc, name, value, help);
}

s7_pointer s7::define_constant_with_documentation(const char *name, s7_pointer value, const char *help) const {
  auto sc = get();
  return s7_define_constant_with_documentation(sc, name, value, help);
}
