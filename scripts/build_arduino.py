"""使用 Arduino CLI 编译与 Arduino IDE 共用的八个示例，不烧录。"""
from pathlib import Path
import argparse
import os
import shutil
import subprocess
import sys

EXAMPLES = ["Power", "Display", "Keys", "WS2812", "AudioPlayback", "AudioRecord", "QWIIC", "Factory"]
FQBN = "esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=cdc,FlashSize=16M,PSRAM=opi,PartitionScheme=app3M_fat9M_16MB,FlashMode=qio"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("examples", nargs="*", metavar="EXAMPLE")
    parser.add_argument("--cli", default=os.environ.get("ARDUINO_CLI", "arduino-cli"))
    parser.add_argument("--config-file", type=Path, help="可选的 Arduino CLI 配置文件")
    parser.add_argument("--deps-dir", type=Path, help="默认 .build/deps；先运行 prepare_dependencies.py")
    parser.add_argument("--fqbn", default=FQBN)
    parser.add_argument("--jobs", type=int, default=4, help="编译并发数，默认 4")
    parser.add_argument("--fail-fast", action="store_true", help="遇到首个失败时停止")
    args = parser.parse_args()
    unknown = set(args.examples) - set(EXAMPLES)
    if unknown:
        parser.error("未知示例：" + ", ".join(sorted(unknown)))
    root = Path(__file__).resolve().parents[1]
    dependencies = (args.deps_dir or root / ".build/deps").resolve()
    if not all((dependencies / name / "library.properties").is_file() for name in ("SensorLib", "LovyanGFX", "ButtonSense")):
        parser.error("缺少依赖，请先运行 python scripts/prepare_dependencies.py")
    logs = root / ".build/logs/arduino"
    logs.mkdir(parents=True, exist_ok=True)
    prefix = [args.cli]
    if args.config_file:
        prefix += ["--config-file", str(args.config_file.resolve())]
    # 固定 Sketch 路径和依赖发现顺序，切换 Demo 时不重复重建同一套库。
    # 这里只生成可丢弃的编译副本，驱动仍直接使用仓库 src/，不维护第二份应用。
    sketch = root / ".build/arduino/sketch/VoiceExample"
    marker = sketch / ".voice-staging"
    if sketch.exists() and (not marker.is_file() or marker.read_text(encoding="utf-8") != "TKeyboardVoice CLI staging\n"):
        parser.error("拒绝覆盖未知暂存目录：" + str(sketch))
    sketch.mkdir(parents=True, exist_ok=True)
    marker.write_text("TKeyboardVoice CLI staging\n", encoding="utf-8")
    prelude = "\n".join("#include <" + header + ">" for header in
                         ("Arduino.h", "VoicePower.h", "VoiceDisplay.h", "VoiceInput.h", "VoiceRGB.h", "VoiceAudio.h")) + "\n"
    results = []
    for name in args.examples or EXAMPLES:
        source = root / "examples" / name
        original = source / (name + ".ino")
        (sketch / "VoiceExample.ino").write_text(prelude + '#line 1 "' + original.as_posix() + '"\n' + original.read_text(encoding="utf-8"), encoding="utf-8")
        shutil.copy2(source / "build_opt.h", sketch / "build_opt.h")
        factory = sketch / "Factory.cpp"
        if name == "Factory":
            shutil.copy2(source / "Factory.cpp", factory)
        elif factory.exists():
            factory.unlink()
        command = prefix + ["compile", "--fqbn", args.fqbn, "--jobs", str(args.jobs),
                            "--library", str(root), "--libraries", str(dependencies),
                            "--build-path", str(root / ".build/arduino/work"),
                            "--output-dir", str(root / ".build/arduino/build" / name),
                            "--warnings", "all", str(sketch)]
        print("Arduino: " + name, flush=True)
        with (logs / (name + ".log")).open("w", encoding="utf-8") as log:
            try:
                code = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT).returncode
            except FileNotFoundError:
                raise SystemExit("找不到 Arduino CLI，请安装或使用 --cli 指定路径。")
        results.append((name, code))
        print(f"  {name}: {'PASS' if code == 0 else 'FAIL'}; log={logs / (name + '.log')}", flush=True)
        if code and args.fail_fast:
            break
    print("Arduino build summary:", flush=True)
    for name, code in results:
        print(f"  {name}: {'PASS' if code == 0 else 'FAIL'}", flush=True)
    return int(any(code for _, code in results))


if __name__ == "__main__":
    sys.exit(main())
