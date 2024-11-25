#include "request_compiler.hpp"
#include "debug.hpp"
#include "gen/s7_scheme_repl_string.hpp"
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

template <typename T, typename Function>
std::pair<const char *, T> eval_with_error_output(s7_scheme *sc, Function f) {
  auto error_port = s7_open_output_string(sc);
  auto previous_error_port = s7_set_current_error_port(sc, error_port);
  auto res = f(sc);
  s7_set_current_error_port(sc, previous_error_port);
  auto error_output = s7_get_output_string(sc, error_port);
  return std::make_pair(error_output, res);
}

inline const char *non_empty_nor_null(const char *s) {
  return s != nullptr && *s != 0 ? s : nullptr;
}

ReplRequestCompiler::ReplRequestCompiler() {
  compile_geiser_request = scheme.make_symbol("compile-geiser-request");
  auto result = eval_with_error_output<const char *>(scheme.get(), [](auto sc) {
    DEBUG_REPL(s7_scheme_repl_string);
    return s7_object_to_c_string(sc,
        s7_load_c_string(sc,
            s7_scheme_repl_string,
            strlen(s7_scheme_repl_string)));
  });
  auto error_output = non_empty_nor_null(result.first);
  if (error_output) {
    gd::printerr(error_output);
  }
}

ReplRequestCompiler::~ReplRequestCompiler() {
  compile_geiser_request = nullptr;
}

error_output_and_response ReplRequestCompiler::eval(const char *compiled_request) {
  auto res = eval_with_error_output<const char *>(scheme.get(),
      [compiled_request](auto sc) {
        auto res = s7_eval_c_string(sc, compiled_request);
        return s7_is_string(res) ? s7_string(res) : s7_object_to_c_string(sc, res);
      });
  return std::make_pair(non_empty_nor_null(res.first), res.second);
}

error_output_and_response ReplRequestCompiler::compile_request(const PackedByteArray &request) {
  auto compile_geiser_request = scheme.value_of(this->compile_geiser_request);
  if (!s7_is_procedure(compile_geiser_request)) {
    return std::make_pair("repl script is missing a compile-geiser-request function!", (const char *)nullptr);
  }

  auto sc = scheme.get();
  auto compile_result =
      eval_with_error_output<s7_pointer>(sc,
          [compile_geiser_request, &request](auto sc) {
            auto args = s7_cons(sc,
                s7_make_string_wrapper_with_length(
                    sc,
                    reinterpret_cast<const char *>(request.ptr()),
                    static_cast<s7_int>(request.size())),
                s7_nil(sc));
            return s7_call_with_location(
                sc,
                compile_geiser_request,
                args,
                __func__,
                __FILE__,
                __LINE__);
          });

  auto compile_error_output = non_empty_nor_null(compile_result.first);
  auto compiled_request = compile_result.second;
  if (!s7_is_string(compiled_request)) {
    DEBUG_REPL(s7_object_to_c_string(sc, compiled_request));
    return std::make_pair(compile_error_output, (const char *)nullptr);
  }

  auto code_string = s7_string(compiled_request);
  DEBUG_REPL("```\n", code_string, "\n```");
  return std::make_pair(compile_error_output, code_string);
}
