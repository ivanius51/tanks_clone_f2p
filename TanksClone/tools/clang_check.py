"""Strict compile check of the game code with clang-cl (syntax + warnings, no linking).

Usage: python tools/clang_check.py [file-or-folder ...] [--jobs N]
Needs LLVM (clang-cl on PATH or in C:/Program Files/LLVM/bin). Exit code 1 when any file has errors or warnings.
"""
import argparse
import concurrent.futures
import os
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ENGINE = os.path.join(ROOT, "cocos2d")
INCLUDES = [
    ENGINE, os.path.join(ENGINE, "cocos"), os.path.join(ENGINE, "external"), os.path.join(ENGINE, "cocos", "audio", "include"),
    os.path.join(ENGINE, "external", "chipmunk", "include", "chipmunk"), os.path.join(ENGINE, "extensions"),
    os.path.join(ENGINE, "cocos", "platform", "win32"), os.path.join(ENGINE, "external", "glfw3", "include", "win32"),
    os.path.join(ENGINE, "external", "win32-specific", "gles", "include", "OGLES"),
    os.path.join(ROOT, "Classes"), ROOT,
]
DEFINES = ["WIN32", "UNICODE", "_UNICODE", "NDEBUG", "TANKS_DEBUG_TOOLS", "_WINDOWS", "_USE_MATH_DEFINES", "GL_GLEXT_PROTOTYPES",
           "CC_ENABLE_CHIPMUNK_INTEGRATION=1", "_CRT_SECURE_NO_WARNINGS", "_SCL_SECURE_NO_WARNINGS", "_HAS_AUTO_PTR_ETC=1",
           "_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING"]
WARNINGS = ["-Wextra", "-Wshadow", "-Wnon-virtual-dtor", "-Wnull-dereference",
            "-Wimplicit-fallthrough", "-Wunreachable-code", "-Wno-unused-parameter", "-Wno-overloaded-virtual", "-Wno-c++98-compat",
            "-Wno-microsoft-include", "-Wno-pragma-pack", "-Wno-unknown-pragmas", "-Wno-missing-field-initializers"]


def find_clang():
    found = shutil.which("clang-cl")
    if found:
        return found
    fallback = r"C:\Program Files\LLVM\bin\clang-cl.exe"
    return fallback if os.path.exists(fallback) else None


def collect(targets):
    files = []
    for target in targets or [os.path.join(ROOT, "Classes")]:
        if os.path.isfile(target):
            files.append(os.path.abspath(target))
            continue
        for folder, _, names in os.walk(target):
            files += [os.path.join(folder, n) for n in names if n.endswith(".cpp") and n != "Pch.cpp"]
    return sorted(files)


def check(clang, path):
    model_file = os.sep + "model" + os.sep in path
    command = [clang, "/nologo", "/std:c++17", "/EHsc", "/MD", "-fsyntax-only", "-m32", "/W4"] + WARNINGS
    command += ["/I" + include for include in INCLUDES] + ["/D" + define for define in DEFINES]
    if not model_file:
        command.append("/FIPch.h")
    command.append(path)
    result = subprocess.run(command, capture_output=True, text=True, cwd=ROOT)
    output = "\n".join(line for line in (result.stdout + result.stderr).splitlines()
                       if line.strip() and "cocos2d" + os.sep not in line.split(":")[0] + ":" + line.split(":")[1:2].__str__())
    own = [line for line in (result.stdout + result.stderr).splitlines()
           if (" warning:" in line or " error:" in line) and os.path.join(ROOT, "Classes").lower() in os.path.abspath(line.split("(")[0]).lower()]
    return path, result.returncode, own, output if result.returncode else ""


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("targets", nargs="*")
    parser.add_argument("--jobs", type=int, default=os.cpu_count() or 4)
    args = parser.parse_args()
    clang = find_clang()
    if not clang:
        print("clang-cl not found: install LLVM (C:/Program Files/LLVM) or add it to PATH")
        return 2
    files = collect(args.targets)
    problems = 0
    seen = set()
    with concurrent.futures.ThreadPoolExecutor(args.jobs) as pool:
        for path, code, own, output in pool.map(lambda f: check(clang, f), files):
            for line in own:
                if line not in seen:
                    seen.add(line)
                    print(line)
            if code != 0 and not own:
                print("FAILED " + os.path.relpath(path, ROOT) + "\n" + output[:2000])
            problems += len(own) + (1 if code != 0 and not own else 0)
    print("clang_check: %d files, %d problems" % (len(files), problems))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
