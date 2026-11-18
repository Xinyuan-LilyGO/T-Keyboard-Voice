"""把原始 WAV 转为三种构建入口共用的只读 C++ 资源；--check 检查是否同步。"""
from pathlib import Path
import argparse
import sys
import wave


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="只检查，不修改生成文件")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    source = root / "assets/66_stereo_48k.wav"
    with wave.open(str(source), "rb") as wav:
        if (wav.getnchannels(), wav.getsampwidth(), wav.getframerate()) != (2, 2, 48000):
            raise SystemExit("测试音频必须为 48 kHz、16-bit、双声道 PCM WAV。")
    data = source.read_bytes()
    lines = ["// 由 scripts/generate_audio.py 生成；请修改原 WAV 后重新生成。\n"]
    lines.extend("    " + ", ".join(f"0x{v:02x}" for v in data[i:i + 16]) + ",\n"
                 for i in range(0, len(data), 16))
    expected = "".join(lines)
    output = root / "src/audio/test_audio.inc"
    if args.check:
        if not output.exists() or output.read_text(encoding="utf-8") != expected:
            print("音频资源未同步，请执行 python scripts/generate_audio.py。")
            return 1
    else:
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(expected, encoding="utf-8", newline="\n")
    print(f"Portable audio resource: {len(data)} bytes PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
