#ifndef GODOT_S7_SCHEME_REPL_REQUEST_COMPILER_HPP
#define GODOT_S7_SCHEME_REPL_REQUEST_COMPILER_HPP

#include "../s7.hpp"
#include <godot_cpp/variant/packed_byte_array.hpp>

typedef std::pair<const char *, const char *> error_output_and_response;

class ReplRequestCompiler {
public:
  ReplRequestCompiler();
  ~ReplRequestCompiler();

public:
  error_output_and_response compile_request(const godot::PackedByteArray &request);
  error_output_and_response eval(const char* compiled_request);

private:
  s7_protected_ptr compile_geiser_request;
  s7 scheme;
};
#endif
