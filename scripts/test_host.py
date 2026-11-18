"""在主机执行纯 C++ 回归测试，支持 GCC、Clang 和 Windows MSVC。"""
from pathlib import Path
import os
import shlex
import shutil
import subprocess
import sys
import tempfile


def msvc_environment():
    base = Path(os.environ.get("ProgramFiles(x86)", "C:/Program Files (x86)")) / "Microsoft Visual Studio"
    candidates = sorted(base.glob("*/BuildTools/Common7/Tools/VsDevCmd.bat"), reverse=True)
    if not candidates:
        return None
    with tempfile.TemporaryDirectory(prefix="voice-msvc-") as folder:
        wrapper = Path(folder) / "environment.cmd"
        wrapper.write_text('@call "' + str(candidates[0]) + '" -no_logo -arch=x64 -host_arch=x64 >nul\n@if errorlevel 1 exit /b 1\n@set\n', encoding="mbcs")
        bootstrap = os.environ.copy()
        bootstrap["PATH"] = str(base / "Installer") + os.pathsep + os.environ.get("PATH", "")
        output = subprocess.check_output(["cmd.exe", "/d", "/c", str(wrapper)], env=bootstrap, text=True, encoding="mbcs", errors="replace")
    environment = os.environ.copy()
    for line in output.splitlines():
        if "=" in line and not line.startswith("="):
            key, value = line.split("=", 1)
            environment[key.upper()] = value
    # Windows 环境变量不区分大小写；删除不同大小写的旧 PATH 防止重复键。
    for key in list(environment):
        if key != "PATH" and key.upper() == "PATH":
            del environment[key]
    return environment


def main():
    root = Path(__file__).resolve().parents[1]
    build = root / ".build" / "host"
    build.mkdir(parents=True, exist_ok=True)
    environment = os.environ.copy()
    explicit = os.environ.get("CXX")
    if explicit:
        compiler = [explicit] if Path(explicit).is_file() else shlex.split(explicit, posix=os.name != "nt")
    elif os.name == "nt":
        if shutil.which("cl"):
            compiler = ["cl"]
        else:
            configured = msvc_environment()
            if configured:
                environment = configured
                compiler = [shutil.which("cl", path=environment.get("PATH")) or "cl"]
            else:
                compiler = [shutil.which("g++") or shutil.which("clang++") or "c++"]
    else:
        compiler = [shutil.which("c++") or shutil.which("g++") or shutil.which("clang++") or "c++"]
    msvc = Path(compiler[0]).stem.lower() == "cl"
    tests = sorted((root / "tests").glob("*_test.cpp"))
    for source in tests:
        executable = build / (source.stem + (".exe" if os.name == "nt" else ""))
        if msvc:
            args = ["/nologo", "/std:c++17", "/W4", "/WX", "/EHsc", "/utf-8", "/I" + str(root / "src"), str(source), "/Fe:" + str(executable), "/Fo:" + str(build / (source.stem + ".obj"))]
        else:
            args = ["-std=c++17", "-Wall", "-Wextra", "-Werror", "-I", str(root / "src"), str(source), "-o", str(executable)]
        print("Testing " + source.name, flush=True)
        subprocess.run(compiler + args, cwd=build, env=environment, check=True)
        subprocess.run([str(executable)], cwd=build, env=environment, check=True)
    print(f"Host tests: {len(tests)}/{len(tests)} PASS")


if __name__ == "__main__":
    try:
        main()
    except FileNotFoundError as error:
        raise SystemExit("未找到原生 C++ 编译器，请安装 GCC/Clang/MSVC 或设置 CXX。原始错误：" + str(error))
