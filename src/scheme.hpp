#ifndef GDS7_H
#define GDS7_H

#include "s7.hpp"
#include "scheme_script.hpp"
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {
class Scheme : public Node {
	GDCLASS(Scheme, Node)

public:
	Scheme();
	~Scheme() override;

	void _ready() override;
	void _process(double delta) override;
  void _enter_tree() override;
	void _exit_tree() override;

	void define(const String &name, const Variant &value, const String &help = "") const;
	void load(const SchemeScript *script) const;
	void load_string(const String &code) const;
	Variant eval(const String &code);
  /**
   * Process an async evaluation request from repl and calls [continuation] with the result.
   */
  void eval_async(const String &code, const Callable &continuation);
  Variant apply(const String &symbol, const Array &args) const;
  void set_prelude(const TypedArray<SchemeScript> &p_prelude) { prelude = p_prelude; }
  [[nodiscard]] TypedArray<SchemeScript> get_prelude() const { return prelude; }
  void set_scheme_script(const Ref<SchemeScript> &p_scheme_script);
  [[nodiscard]] Ref<SchemeScript> get_scheme_script() const { return scheme_script; };

  [[nodiscard]] const s7 &get_s7() const { return scheme; }

protected:
  static void _bind_methods();

private:
  void load_prelude();
  void load_script();

private:
	TypedArray<SchemeScript> prelude;
	Ref<SchemeScript> scheme_script;
	s7_protected_ptr _process_symbol;
	s7 scheme;
};

} // namespace godot

#endif
