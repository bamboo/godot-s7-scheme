#!/usr/bin/env python
import os
import sys
from methods import print_error

# -------------------------- Utility Functions --------------------------
def embed_file(target, source, env):
    """
    Embeds the content of <source> file into a <target> header file as a const char* constant.
    """
    target_path = str(target[0])
    source_path = str(source[0])

    # Read the content of the source file
    with open(source_path, 'r') as src_file:
        file_content = src_file.read()

    # Generate a unique header guard based on the file name
    target_file_name = os.path.basename(target_path)
    header_guard = target_file_name.replace('-', '_').replace('.', '_').upper()

    # Write the output header file
    constant_name = os.path.splitext(target_file_name)[0].replace('-', '_')
    with open(target_path, 'w') as target_file:
        target_file.write(f"""
#ifndef {header_guard}
#define {header_guard}

const char* {constant_name} = R"({file_content})";

#endif // {header_guard}
""")

def is_submodule_initialized(path):
  return os.path.isdir(path) and os.listdir(path)

# -------------------------- Build definition --------------------------

lib_name = "godot-s7-scheme"
project_dir = "demo"

local_env = Environment(tools=["default"], PLATFORM="")

customs = ["custom.py"]
customs = [os.path.abspath(path) for path in customs]

opts = Variables(customs, ARGUMENTS)
opts.Update(local_env)

Help(opts.GenerateHelpText(local_env))

if not is_submodule_initialized('godot-cpp'):
    print_error("""godot-cpp is not available within this folder, as Git submodules haven't been initialized.
Run the following command to download godot-cpp:

    git submodule update --init --recursive""")
    sys.exit(1)

env = SConscript("godot-cpp/SConstruct", {"env": local_env.Clone(), "customs": customs})
env.Append(
    CPPPATH=["src/", "s7/"],
    CPPDEFINES={
        "DISABLE_DEPRECATED": "1",
        "DISABLE_AUTOLOAD": "1",
        "WITH_C_LOADER": "0",
        "WITH_MULTITHREAD_CHECKS": "0",
        "WITH_SYSTEM_EXTRAS": "0",
        "HAVE_COMPLEX_NUMBERS": "0" if env["platform"] == "windows" else "1"
    }
)

sources = [
    Glob("src/*.cpp"),
    Glob("src/repl/*.cpp"),
    Glob("s7/s7.c")
]

if env["target"] in ["editor", "template_debug"]:
    try:
        doc_data = env.GodotCPPDocData("src/gen/doc_data.gen.cpp", source=Glob("doc_classes/*.xml"))
        sources.append(doc_data)
    except AttributeError:
        print("Not including class reference as we're targeting a pre-4.3 baseline.")

file = "{}{}{}".format(lib_name, env["suffix"], env["SHLIBSUFFIX"])
file_path = ""

if env["platform"] == "macos" or env["platform"] == "ios":
    file_path = "{}.framework/".format(env["platform"])
    file = "{}.{}.{}".format(lib_name, env["platform"], env["target"])

library_file = "bin/{}/{}{}".format(env["platform"], file_path, file)
library = env.SharedLibrary(
    library_file,
    source=sources,
)

copy = env.InstallAs("{}/bin/{}/{}lib{}".format(project_dir, env["platform"], file_path, file), library)

embed_scheme_repl = env.Command(
    target="src/repl/gen/s7_scheme_repl_string.hpp",
    source="demo/addons/s7/s7_scheme_repl.scm",
    action=embed_file
)

default_args = [embed_scheme_repl, library, copy]
Default(*default_args)
