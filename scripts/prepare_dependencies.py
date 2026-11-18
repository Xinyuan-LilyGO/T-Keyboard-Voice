"""为 Arduino IDE/CLI 和 ESP-IDF 准备固定提交的公共依赖，不修改系统库目录。"""
from pathlib import Path, PurePosixPath
import argparse
import json
import shutil
import tempfile
import urllib.request
import zipfile

DEPENDENCIES = {
    "SensorLib": ("lewisxhe/SensorLib", "2b9e591f245e447d3d00ec8798c3f49b897882d9"),
    "LovyanGFX": ("lovyan03/LovyanGFX", "3f78b7058fca0fd1fc1ddb32f75a18262e01d440"),
    "ButtonSense": ("lbuque/ButtonSense", "b1aeb22a73c523b37ebd11bee54000b77484f76c"),
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, help="依赖目录，默认 .build/deps")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    output = (args.output or root / ".build/deps").resolve()
    output.mkdir(parents=True, exist_ok=True)
    downloads = root / ".build/downloads"
    downloads.mkdir(parents=True, exist_ok=True)
    for name, (repo, revision) in DEPENDENCIES.items():
        target = output / name
        marker = target / ".voice-dependency.json"
        expected = {"repository": repo, "revision": revision}
        if marker.exists() and json.loads(marker.read_text(encoding="utf-8")) == expected:
            print(name + ": cached", flush=True)
            continue
        if target.exists():
            raise SystemExit(f"拒绝覆盖未知或不同版本的依赖目录：{target}")
        # 每个下载包及其解压内容位于独立的新目录，不在其中执行脚本。
        work = Path(tempfile.mkdtemp(prefix=name + "-", dir=downloads))
        archive = work / "source.zip"
        url = f"https://codeload.github.com/{repo}/zip/{revision}"
        print("Downloading " + name + " " + revision, flush=True)
        with urllib.request.urlopen(url, timeout=120) as response, archive.open("wb") as stream:
            shutil.copyfileobj(response, stream)
        extracted = work / "unpacked"
        extracted.mkdir()
        with zipfile.ZipFile(archive) as package:
            for entry in package.infolist():
                path = PurePosixPath(entry.filename)
                if path.is_absolute() or ".." in path.parts or ":" in entry.filename or "\\" in entry.filename:
                    raise SystemExit("归档包含不安全路径：" + entry.filename)
                if (entry.external_attr >> 16) & 0o170000 == 0o120000:
                    raise SystemExit("归档包含符号链接：" + entry.filename)
            package.extractall(extracted)
        children = list(extracted.iterdir())
        if len(children) != 1 or not children[0].is_dir() or not (children[0] / "src").is_dir():
            raise SystemExit("归档结构不符合预期：" + name)
        shutil.copytree(children[0], target)
        marker.write_text(json.dumps(expected, indent=2) + "\n", encoding="utf-8")
        print(name + ": ready", flush=True)
    print("Dependencies: " + str(output))


if __name__ == "__main__":
    main()
