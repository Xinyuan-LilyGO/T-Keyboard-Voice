"""生成供 Arduino IDE“添加 .ZIP 库”安装的四个包，不包含 SDK 和构建缓存。"""
from pathlib import Path
import os
import zipfile


def main():
    root = Path(__file__).resolve().parents[1]
    output = root / ".build/arduino/zips"
    sources = {"TKeyboardVoice": root}
    sources.update({name: root / ".build/deps" / name for name in ("SensorLib", "LovyanGFX", "ButtonSense")})
    if not all((source / "library.properties").is_file() for source in sources.values()):
        raise SystemExit("请先运行 python scripts/prepare_dependencies.py。")
    output.mkdir(parents=True, exist_ok=True)
    for name, source in sources.items():
        files = []
        for directory in ("src", "examples", "docs", "assets") if name == "TKeyboardVoice" else ("src",):
            folder = source / directory
            if folder.is_dir():
                for current, directories, filenames in os.walk(folder):
                    # IDE/CLI 导出时可能在示例旁生成 build/，安装包只能包含源文件和资源。
                    directories[:] = [item for item in directories if item not in {"build", ".pio", ".build", ".git", "__pycache__"}]
                    files.extend(Path(current) / item for item in filenames if not item.endswith((".ino.cpp", ".pyc")))
        files.extend(path for path in source.iterdir() if path.is_file() and
                     (path.name in {"library.properties", "keywords.txt", "THIRD_PARTY_NOTICES.md"}
                      or path.name.lower().startswith(("license", "readme"))))
        target = output / (name + ".zip")
        with zipfile.ZipFile(target, "w", zipfile.ZIP_DEFLATED) as package:
            for path in sorted(files):
                package.write(path, str(Path(name) / path.relative_to(source)))
        print(str(target))


if __name__ == "__main__":
    main()
