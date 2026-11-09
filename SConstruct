# Copyright (c) 2026 Riverside Research
# -*- python -*-


import os
import subprocess
import sys
import tempfile

default_install_dir = "/usr/local"

vars = Variables(None, ARGUMENTS)
vars.Add(
    PathVariable(
        "DESTDIR",
        "Root directory to install in (useful for packaging scripts)",
        None,
        PathVariable.PathIsDirCreate,
    )
)
vars.Add(
    PathVariable("prefix", "Where to install in the FHS", "/usr/local", PathVariable.PathAccept)
)
vars.Add(
    ListVariable("bindings", "Language bindings to build", "none", ["python", "java", "cpp", "go"])
)
vars.Add("python", "Python interpreter", "python3")
vars.Add(
    EnumVariable(
        "platform",
        "Platform provider (auto, portable, posix, windows, or custom)",
        "auto",
        allowed_values=("auto", "portable", "posix", "windows", "custom"),
    )
)
vars.Add("platform_source", "Provider source required by platform=custom", "")
vars.Add(BoolVariable("platform_threads", "Custom provider supports threaded graph ownership", False))

tools = ["default", "scanreplace"]
requested_cc = ARGUMENTS.get("CC") or os.getenv("CC")

# clang-cl retains the normal Windows archiver/linker tool selection.
if os.path.basename(requested_cc or "") == "clang-cl":
    pass
elif requested_cc == "clang":
    tools.append("clang")
else:
    # try to detect if cc happens to be clang by inspecting --version
    cc = requested_cc or "cc"
    ver = subprocess.run([cc, "--version"], capture_output=True, check=False).stdout
    if b"clang" in ver.split():
        tools.append("clang")
        os.environ["CC"] = cc  # make sure we call it as we saw it

envvars = {"PATH": os.environ["PATH"]}
if "PKG_CONFIG_PATH" in os.environ:
    envvars["PKG_CONFIG_PATH"] = os.environ["PKG_CONFIG_PATH"]
for tempvar in ("TEMP", "TMP"):
    if tempvar in os.environ:
        envvars[tempvar] = os.environ[tempvar]

env = Environment(ENV=envvars, variables=vars, tools=tools, toolpath=["tools"])
env["HAMMER_LIBRARY_PATH_ENV"] = "DYLD_LIBRARY_PATH" if sys.platform == "darwin" else "LD_LIBRARY_PATH"

selected_platform = env["platform"]
if selected_platform == "auto":
    if os.name == "nt" or sys.platform.startswith("win"):
        selected_platform = "windows"
    elif sys.platform.startswith(("linux", "darwin", "freebsd", "netbsd", "openbsd")):
        selected_platform = "posix"
    else:
        selected_platform = "portable"
if selected_platform == "custom" and not env["platform_source"]:
    print("platform=custom requires platform_source=<provider.c>", file=sys.stderr)
    Exit(1)

env["H_PLATFORM_PROVIDER"] = selected_platform
env["H_PLATFORM_POSIX"] = int(selected_platform == "posix")
env["H_PLATFORM_WINDOWS"] = int(selected_platform == "windows")
env["H_PLATFORM_THREADS"] = int(
    selected_platform in ("posix", "windows")
    or (selected_platform == "custom" and bool(env["platform_threads"]))
)
env["PLATFORM_SOURCE"] = (
    env["platform_source"] if selected_platform == "custom" else f"platform_{selected_platform}.c"
)
env["PLATFORM_PC_LIBS"] = "-pthread" if selected_platform == "posix" else ""


def calcInstallPath(*elements):
    path = os.path.abspath(os.path.join(*map(env.subst, elements)))
    if "DESTDIR" in env:
        path = os.path.join(env["DESTDIR"], os.path.relpath(path, start="/"))
    return path


rel_prefix = not os.path.isabs(env["prefix"])
env["prefix"] = os.path.abspath(env["prefix"])
if "DESTDIR" in env:
    env["DESTDIR"] = os.path.abspath(env["DESTDIR"])
    if rel_prefix:
        print(
            "--!!-- You used a relative prefix with a DESTDIR. This is probably not what you",
            file=sys.stderr,
        )
        print("--!!-- you want; files will be installed in", file=sys.stderr)
        print("--!!--    {}".format(calcInstallPath("$prefix")), file=sys.stderr)


env["libpath"] = calcInstallPath("$prefix", "lib")
env["incpath"] = calcInstallPath("$prefix", "include", "hammer")
env["parsersincpath"] = calcInstallPath("$prefix", "include", "hammer", "parsers")
env["backendsincpath"] = calcInstallPath("$prefix", "include", "hammer", "backends")
env["pkgconfigpath"] = calcInstallPath("$prefix", "lib", "pkgconfig")

# DESTDIR is only a staging location. The installed Mach-O identity must always
# name the library's final location under the logical prefix.
if sys.platform == "darwin":
    env.Append(
        SHLINKFLAGS=[
            f"-Wl,-install_name,{os.path.join(env['prefix'], 'lib', 'libhammer.dylib')}"
        ]
    )

with open("VERSION", "r") as f:
    version = f.read().strip()

# Export version for use in sub-SConscripts
env["VERSION"] = version

pkgconfig_file = env.ScanReplace(
    "libhammer.pc.in", VERSION=version, PLATFORM_LIBS=env["PLATFORM_PC_LIBS"]
)
env.Depends(pkgconfig_file, "VERSION")
AlwaysBuild(pkgconfig_file)

AddOption(
    "--variant",
    dest="variant",
    nargs=1,
    type="choice",
    choices=["debug", "opt"],
    default="opt",
    action="store",
    help="Build variant (debug or opt)",
)

AddOption(
    "--coverage",
    dest="coverage",
    default=False,
    action="store_true",
    help="Build with coverage instrumentation",
)

AddOption(
    "--force-debug",
    dest="force_debug",
    default=False,
    action="store_true",
    help="Build with debug symbols, even in the opt variant",
)

AddOption(
    "--gprof",
    dest="gprof",
    default=False,
    action="store_true",
    help="Build with profiling instrumentation for gprof",
)

AddOption(
    "--in-place",
    dest="in_place",
    default=False,
    action="store_true",
    help="Build in-place, rather than in the build/<variant> tree",
)

AddOption(
    "--no-tests", dest="with_tests", default=True, action="store_false", help="Do not build tests"
)

AddOption(
    "--fPIC",
    dest="fpic",
    default=False,
    action="store_true",
    help="compile with the fPIC flag for use with shared objects",
)

env["CC"] = requested_cc or env["CC"]
env["CXX"] = ARGUMENTS.get("CXX") or os.getenv("CXX") or env["CXX"]
env["CFLAGS"] = ARGUMENTS.get("CFLAGS") or os.getenv("CFLAGS") or env["CFLAGS"]
env["LINKFLAGS"] = ARGUMENTS.get("LDFLAGS") or os.getenv("LDFLAGS") or env["LINKFLAGS"]

if selected_platform == "windows":
    env.Append(CCFLAGS=["/std:c11", "/W4", "/WX"])
else:
    env.MergeFlags(
        "-std=c99 -Wall -Wextra -Werror -Wno-unused-parameter "
        "-Wno-attributes -Wno-unused-variable"
    )

if selected_platform == "posix":
    env.MergeFlags("-D_POSIX_C_SOURCE=200809L -pthread")
    with tempfile.TemporaryDirectory(prefix="hammer-librt-") as probe_dir:
        probe_source = os.path.join(probe_dir, "probe.c")
        probe_binary = os.path.join(probe_dir, "probe")
        with open(probe_source, "w") as probe:
            probe.write(
                "#define _POSIX_C_SOURCE 200809L\n"
                "#include <time.h>\n"
                "int main(void) { struct timespec t; return clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t); }\n"
            )
        try:
            needs_librt = subprocess.run(
                [os.getenv("CC") or env["CC"], probe_source, "-o", probe_binary],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                check=False,
            ).returncode != 0
        except OSError:
            needs_librt = False
    if needs_librt:
        env.Append(LIBS=["rt"])
        env["PLATFORM_PC_LIBS"] += " -lrt"

if GetOption("coverage"):
    if selected_platform == "windows":
        print("--coverage is not supported by platform=windows", file=sys.stderr)
        Exit(1)
    env.Append(CCFLAGS=["--coverage"], LDFLAGS=["--coverage"], LINKFLAGS=["--coverage"])
    if env["CC"] == "gcc":
        env.Append(LIBS=["gcov"])
    else:
        env.ParseConfig("llvm-config --ldflags")

if GetOption("force_debug"):
    env.Append(CCFLAGS=["/Zi" if selected_platform == "windows" else "-g"])

if GetOption("gprof"):
    if selected_platform == "windows":
        print("--gprof is not supported by platform=windows", file=sys.stderr)
        Exit(1)
    if env["CC"] == "gcc" and env["CXX"] == "g++":
        env.Append(CCFLAGS=["-pg"], LDFLAGS=["-pg"], LINKFLAGS=["-pg"])
        env["GPROF"] = 1
    else:
        print("Can only use gprof with gcc")
        Exit(1)

if GetOption("fpic"):
    if selected_platform == "windows":
        print("--fPIC is not supported by platform=windows", file=sys.stderr)
        Exit(1)
    if env["CC"] == "gcc":
        env.Append(CCFLAGS=["-fPIC"])
    else:
        print("Can only use fPIC with gcc")
        Exit(1)

dbg = env.Clone(VARIANT="debug")
dbg.Append(CCFLAGS=["/Zi" if selected_platform == "windows" else "-g"])

opt = env.Clone(VARIANT="opt")
opt.Append(CCFLAGS=["/O2" if selected_platform == "windows" else "-O3"])

if GetOption("variant") == "debug":
    env = dbg
else:
    env = opt

env["ENV"].update(x for x in os.environ.items() if x[0].startswith("CCC_"))

# rootpath = env['ROOTPATH'] = os.path.abspath('.')
# env.Append(CPPPATH=os.path.join('#', 'hammer'))

testruns = []
binding_results = []      # [(display_name, results_file_path), ...]
binding_test_stamps = []  # [SCons node of each binding test stamp, ...]

targets = ["$libpath", "$incpath", "$parsersincpath", "$backendsincpath", "$pkgconfigpath"]

Export("env")
Export("testruns")
Export("targets")
Export("binding_results")
Export("binding_test_stamps")
Export("selected_platform")

if not GetOption("in_place"):
    env["BUILD_BASE"] = "build/$VARIANT"
    lib = env.SConscript(["src/SConscript"], variant_dir="$BUILD_BASE/src")
    if selected_platform != "windows":
        env.Alias(
            "examples",
            env.SConscript(["examples/old/SConscript"], variant_dir="$BUILD_BASE/examples"),
        )
else:
    env["BUILD_BASE"] = "."
    lib = env.SConscript(["src/SConscript"])
    if selected_platform != "windows":
        env.Alias(env.SConscript(["examples/old/SConscript"]))

for testrun in testruns:
    env.Alias("test", testrun)

if binding_results:
    _br = list(binding_results)

    def _print_summary(target, source, env, br=_br):
        rows = []
        for name, rf in br:
            try:
                with open(env.subst(rf)) as result_file:
                    parts = result_file.read().strip().split()
                rows.append((name, int(parts[1]), int(parts[2])))
            except (OSError, IndexError, ValueError):
                rows.append(("N/A", 0, 0))
        if not rows:
            return 0
        w = max(len(r[0]) for r in rows)
        tp = sum(r[1] for r in rows)
        tf = sum(r[2] for r in rows)
        print("\nTest Results:")
        for name, p, f in rows:
            t = p + f
            status = "passed" if not f else "FAILED"
            print(f"  {name:<{w}} : {p}/{t} {status}")
        print("  " + "\u2500" * (w + 18))
        total = tp + tf
        status = "passed" if not tf else "FAILED"
        print(f"  {'Total':<{w}} : {tp}/{total} {status}\n")
        return 0

    build_base = env.subst("$BUILD_BASE")
    summary = env.Command(
        os.path.join(build_base, "src", "binding_tests_summary.stamp"),
        binding_test_stamps,
        Action(_print_summary, strfunction=lambda *a: ""),
    )
    AlwaysBuild(summary)
    env.Alias("test", summary)

# Add gcov target to generate coverage files in build directory
if GetOption("coverage"):
    build_base = env.subst("$BUILD_BASE")
    # Create a command that runs gcov from the build directory
    # This ensures .gcov files are generated in the build directory
    gcov_script = (
        f"""
import os
import subprocess
build_base = "{build_base}"
if os.path.exists(build_base):
    for root, dirs, files in os.walk(build_base):
        for file in files:
            if file.endswith('.gcda'):
                gcov_file = os.path.join(root, file)
                gcov_dir = os.path.dirname(gcov_file)
                # Run gcov from the directory containing the .gcda file
                # This ensures .gcov files are created in the same directory
                # Use -o to specify output directory and pass just the filename
                subprocess.run(['gcov', '-o', gcov_dir, os.path.basename(gcov_file)], 
                             cwd=gcov_dir, 
                             stdout=subprocess.DEVNULL, 
                             stderr=subprocess.DEVNULL)
"""
    )
    env.Command("gcov", [], f"python -c {gcov_script!r}")

env.Alias("install", targets)


def uninstall_action(target, source, env):
    import glob as _glob

    lib_files = (
        _glob.glob(os.path.join(env["libpath"], "libhammer.so*"))
        + _glob.glob(os.path.join(env["libpath"], "libhammer.dylib*"))
        + [os.path.join(env["libpath"], "libhammer.a")]
    )

    inc_files = [
        os.path.join(env["incpath"], h)
        for h in [
            "hammer.h",
            "hammer_auto_source.h",
            "allocator.h",
            "compiler_specifics.h",
            "glue.h",
            "internal.h",
            "platform.h",
            "platform_config.h",
        ]
    ] + [
        os.path.join(env["parsersincpath"], "parser_internal.h"),
        os.path.join(env["backendsincpath"], "missing.h"),
        os.path.join(env["backendsincpath"], "params.h"),
    ]

    pc_file = [os.path.join(env["pkgconfigpath"], "libhammer.pc")]

    for path in lib_files + inc_files + pc_file:
        if os.path.exists(path) or os.path.islink(path):
            os.remove(path)
            print(f"Removed: {path}")
        else:
            print(f"Not found (skipping): {path}")

    for d in [env["backendsincpath"], env["parsersincpath"], env["incpath"]]:
        try:
            os.rmdir(d)
            print(f"Removed directory: {d}")
        except OSError:
            pass


uninstall = env.Command("uninstall", [], uninstall_action)
env.Alias("uninstall", uninstall)
AlwaysBuild(uninstall)
