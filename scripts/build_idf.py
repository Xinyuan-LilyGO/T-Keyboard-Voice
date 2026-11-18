"""在 ESP-IDF 5.5.x 已导出的终端中构建八个示例（Arduino 组件方式），不烧录。"""
from pathlib import Path
import argparse
import os
import shutil
import subprocess
import sys

EXAMPLES = ["Factory", "Power", "Display", "Keys", "WS2812", "AudioPlayback", "AudioRecord", "QWIIC"]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("examples", nargs="*", metavar="EXAMPLE")
    parser.add_argument("--fail-fast", action="store_true", help="遇到首个失败时停止")
    args = parser.parse_args()
    if set(args.examples) - set(EXAMPLES):
        parser.error("未知示例：" + ", ".join(sorted(set(args.examples) - set(EXAMPLES))))
    root = Path(__file__).resolve().parents[1]
    sdk = os.environ.get("IDF_PATH")
    if not sdk or not (Path(sdk) / "tools/idf.py").is_file():
        parser.error("请先进入 ESP-IDF 5.5.x 终端，或执行对应版本的 export 脚本。")
    python = sys.executable
    virtual = os.environ.get("IDF_PYTHON_ENV_PATH")
    if virtual:
        candidate = Path(virtual) / ("Scripts/python.exe" if os.name == "nt" else "bin/python")
        if candidate.is_file():
            python = str(candidate)
    environment = os.environ.copy()
    if virtual:
        environment["IDF_PYTHON_ENV_PATH"] = str(Path(virtual).resolve())
    if os.name == "nt":
        # 调用原生 Windows Python/工具链，不把外层 Git Bash 标识传给 idf.py。
        environment.pop("MSYSTEM", None)
    output = root / ".build/idf"
    build = output / "work"
    logs = root / ".build/logs/idf"
    logs.mkdir(parents=True, exist_ok=True)
    results = []
    for name in args.examples or EXAMPLES:
        command = [python, str(Path(sdk) / "tools/idf.py"), "-C", str(root / "idf"), "-B", str(build),
                   "-D", "IDF_TARGET=esp32s3", "-D", "VOICE_EXAMPLE=" + name,
                   "-D", "SDKCONFIG=" + str(build / "sdkconfig"), "build"]
        print("ESP-IDF: " + name, flush=True)
        with (logs / (name + ".log")).open("w", encoding="utf-8") as log:
            code = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, env=environment).returncode
        if code == 0:
            target = output / name
            target.mkdir(parents=True, exist_ok=True)
            for relative in ("t_keyboard_voice.bin", "t_keyboard_voice.elf", "bootloader/bootloader.bin",
                             "partition_table/partition-table.bin", "flasher_args.json", "sdkconfig"):
                source = build / relative
                if not source.is_file():
                    print("Missing artifact: " + str(source), flush=True)
                    code = 1
                    break
                destination = target / relative
                destination.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(source, destination)
        results.append((name, code))
        print(f"  {name}: {'PASS' if code == 0 else 'FAIL'}; log={logs / (name + '.log')}", flush=True)
        if code and args.fail_fast:
            break
    print("ESP-IDF build summary:", flush=True)
    for name, code in results:
        print(f"  {name}: {'PASS' if code == 0 else 'FAIL'}", flush=True)
    return int(any(code for _, code in results))


if __name__ == "__main__":
    sys.exit(main())
