"""依次构建全部示例，保存日志并返回真实失败状态。"""
from pathlib import Path
import argparse
import os
import subprocess
import sys

ENVIRONMENTS = ["power", "display", "keys", "ws2812", "audio_playback", "audio_record", "qwiic", "factory"]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("environments", nargs="*", metavar="ENV", help="环境名；省略则构建全部示例")
    parser.add_argument("--clean", action="store_true", help="先清理对应环境再编译")
    args = parser.parse_args()
    invalid = set(args.environments) - set(ENVIRONMENTS)
    if invalid:
        parser.error("未知环境：" + ", ".join(sorted(invalid)))
    root = Path(__file__).resolve().parents[1]
    logs = root / ".build" / "logs"
    logs.mkdir(parents=True, exist_ok=True)
    # 避免调用该脚本的 Python 与安装 PlatformIO 的 Python 不一致。
    pio = os.environ.get("PIO", "pio")
    results = []
    for name in args.environments or ENVIRONMENTS:
        command = [pio, "run", "-d", str(root), "-e", name]
        if args.clean:
            subprocess.run(command + ["-t", "clean"], check=True)
        print("Building " + name, flush=True)
        with (logs / (name + ".log")).open("w", encoding="utf-8") as log:
            try:
                process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, encoding="utf-8", errors="replace")
            except FileNotFoundError:
                raise SystemExit("找不到 PlatformIO，请安装后将 pio 加入 PATH，或设置 PIO 为可执行文件路径。")
            for line in process.stdout:
                log.write(line)
                print(line, end="", flush=True)
            code = process.wait()
        results.append((name, code))
    print("\nBuild summary:")
    for name, code in results:
        print(f"  {name}: {'PASS' if code == 0 else 'FAIL'}")
    return int(any(code for _, code in results))


if __name__ == "__main__":
    sys.exit(main())
