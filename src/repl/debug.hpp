#ifndef GODOT_S7_SCHEME_REPL_DEBUG_HPP
#define GODOT_S7_SCHEME_REPL_DEBUG_HPP

#include <godot_cpp/variant/utility_functions.hpp>

using gd = godot::UtilityFunctions;

#define DEBUG_REPL_INTERACTIONS 0

#if DEBUG_REPL_INTERACTIONS
#define DEBUG_REPL(...) gd::print(__VA_ARGS__)
#else
#define DEBUG_REPL(...) 0
#endif

#endif
