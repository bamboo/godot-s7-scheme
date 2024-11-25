#ifndef GODOT_S7_SCHEME_S7_HPP
#define GODOT_S7_SCHEME_S7_HPP

#include <s7.h>
#include <godot_cpp/variant/variant.hpp>
#include <memory>

typedef void (*s7_output_port_function_t)(s7_scheme *sc, uint8_t c, s7_pointer port);

typedef std::shared_ptr<s7_cell> s7_protected_ptr;

inline s7_protected_ptr s7_gc_protected(s7_scheme *sc, s7_pointer p) {
  auto l = s7_gc_protect(sc, p);
  s7_protected_ptr ptr(p, [sc, l]([[maybe_unused]] auto p) { s7_gc_unprotect_at(sc, l); });
  return ptr;
}

class s7_scheme_context;

class s7 {
public:
  s7(const s7 &other) = default;
  s7();

  [[nodiscard]] s7_scheme *get() const { return scheme.get(); };

  s7_pointer define(const char *name, s7_pointer value, const char *documentation) const;
  s7_pointer define_constant_with_documentation(
      const char *name,
      s7_pointer value,
      const char *documentation) const;
  [[nodiscard]] s7_pointer eval(const godot::String &code) const;
  void load_string(const godot::String &code) const;
  void set_current_error_port_function(s7_output_port_function_t f) const;

  s7_protected_ptr make_symbol(const char *name) const {
    auto sc = get();
    return s7_gc_protected(sc, s7_make_symbol(sc, name));
  }

  s7_pointer value_of(const s7_protected_ptr &symbol) const {
    return s7_symbol_value(get(), symbol.get());
  }

  template <typename S>
  s7_pointer call_optional(S what) const {
    auto sc = get();
    auto proc = _scheme_resolve(sc, what);
    return s7_is_procedure(proc)
        ? s7_call_with_location(sc, proc, s7_nil(sc), __func__, __FILE__, __LINE__)
        : s7_unspecified(sc);
  }

  template <typename S, typename T>
  s7_pointer call(S what, T arg) const {
    auto sc = get();
    auto proc = _scheme_resolve(sc, what);
    return s7_call_with_location(sc,
        proc,
        s7_cons(sc, _scheme_value_of(sc, arg), s7_nil(sc)),
        __func__,
        __FILE__,
        __LINE__);
  }

private:
  static s7_pointer _scheme_value_of(s7_scheme *sc, double arg) {
    return s7_make_real(sc, arg);
  }

  static s7_pointer _scheme_value_of(s7_scheme *sc, int32_t arg) {
    return s7_make_integer(sc, arg);
  }

  static s7_pointer _scheme_resolve(s7_scheme *sc, s7_pointer symbol) {
    return s7_symbol_value(sc, symbol);
  }

  static s7_pointer _scheme_resolve(s7_scheme *sc, const char *name) {
    return s7_name_to_value(sc, name);
  }

  std::shared_ptr<s7_scheme> scheme;
  std::shared_ptr<s7_scheme_context> scheme_context;
};
#endif //GODOT_S7_SCHEME_S7_HPP
