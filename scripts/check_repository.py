"""检查示例完整性、共享层隔离、元数据和本地文档链接。"""
from pathlib import Path
import configparser
import json
import os
import re
import sys


def main():
    root = Path(__file__).resolve().parents[1]
    expected = {"Power", "Display", "Keys", "WS2812", "AudioPlayback", "AudioRecord", "QWIIC", "Factory"}
    actual = {path.name for path in (root / "examples").iterdir() if path.is_dir()}
    errors = []
    for directory in [root] + [root / "examples" / name for name in sorted(expected) if (root / "examples" / name).is_dir()]:
        filenames = {path.name for path in directory.iterdir() if path.is_file()}
        for filename in ("README.md", "README_CN.md"):
            if filename not in filenames:
                errors.append(f"缺少规范命名的双语文档：{directory.relative_to(root)}/{filename}")
        if {"README.md", "README_CN.md"} <= filenames:
            english = (directory / "README.md").read_text(encoding="utf-8")
            chinese = (directory / "README_CN.md").read_text(encoding="utf-8")
            if "[简体中文](README_CN.md)" not in english or "[English](README.md)" not in chinese:
                errors.append(f"双语文档缺少语言切换：{directory.relative_to(root)}")
            if re.findall(r"```bash\n(.*?)```", english, re.S) != re.findall(r"```bash\n(.*?)```", chinese, re.S):
                errors.append(f"双语文档构建命令不一致：{directory.relative_to(root)}")
    if actual != expected:
        errors.append(f"示例目录不匹配：{actual ^ expected}")
    for name in expected:
        for filename in (name + ".ino", "README.md", "build_opt.h"):
            if not (root / "examples" / name / filename).is_file():
                errors.append(f"缺少 examples/{name}/{filename}")
        options = root / "examples" / name / "build_opt.h"
        expected_options = {"-DSENSORLIB_EXCLUDE_ALL=1", "-DSENSORLIB_EXCLUDE_PMIC=0", "-DSENSORLIB_EXCLUDE_PMIC_AXP2101=0"}
        if options.is_file() and set(options.read_text(encoding="utf-8").split()) != expected_options:
            errors.append(f"Arduino 编译选项不一致：{name}/build_opt.h")
    factory = (root / "examples/Factory/Factory.cpp").read_text(encoding="utf-8")
    count_match = re.search(r"constexpr size_t TEST_COUNT = (\d+);", factory)
    if not count_match or int(count_match.group(1)) != 18:
        errors.append("Factory 测试总数应为 18")
    for catalog in ("TEST_NAMES", "TEST_GUIDES"):
        match = re.search(catalog + r"\[\]\s*=\s*\{(.*?)\};", factory, re.S)
        if not match or len(re.findall(r'"[^"]*"', match.group(1))) != 18:
            errors.append("Factory 测试目录数量不一致：" + catalog)
    for language in ("README.md", "README_CN.md"):
        text = (root / "examples/Factory" / language).read_text(encoding="utf-8")
        rows = [int(number) for number in re.findall(r"^\| (\d+) ·", text, re.M)]
        if rows != list(range(1, 19)):
            errors.append("Factory 使用说明的编号必须连续为 1～18：" + language)
    for path in (root / "src").glob("*"):
        if path.suffix not in (".h", ".cpp"):
            continue
        text = path.read_text(encoding="utf-8")
        if re.search(r"\b(?:currentTest|TEST_NAMES|setResult)\b|void\s+(?:setup|loop)\s*\(", text):
            errors.append(f"共享层依赖应用状态：{path.name}")
        if re.search(r'#include\s*[<"][^>"\n]*\.ino[>"]', text):
            errors.append(f"共享层包含示例入口：{path.name}")
    # 在进入目录前剪枝，避免扫描已下载的 SDK、隔离归档及构建缓存。
    markdown = []
    for directory, folders, files in os.walk(root):
        folders[:] = [name for name in folders if name not in {".pio", ".git", ".build", "managed_components", "build", "__pycache__", ".vscode"}]
        markdown.extend(Path(directory) / name for name in files if name.endswith(".md"))
    for path in markdown:
        for link in re.findall(r"\]\(([^\s)]+)\)", path.read_text(encoding="utf-8")):
            if "://" in link or link.startswith(("#", "mailto:")):
                continue
            target = link.split("#", 1)[0]
            if target and not (path.parent / target).exists():
                errors.append(f"无效链接：{path.relative_to(root)} -> {target}")
    json.loads((root / "library.json").read_text(encoding="utf-8"))
    json.loads((root / "boards/t_keyboard_voice_esp32s3.json").read_text(encoding="utf-8"))
    config = configparser.ConfigParser(interpolation=None)
    config.read(root / "platformio.ini", encoding="utf-8")
    configured = {config[section].get("custom_example") for section in config.sections() if section.startswith("env:")}
    if configured != expected:
        errors.append("构建环境与示例目录不一致")
    task_config = json.loads((root / ".vscode/tasks.json").read_text(encoding="utf-8"))
    editor_settings = json.loads((root / ".vscode/settings.json").read_text(encoding="utf-8"))
    task_labels = {task["label"] for task in task_config.get("tasks", [])}
    choices = next((item for item in task_config.get("inputs", []) if item.get("id") == "pioEnvironment"), {})
    environment_names = {section[4:] for section in config.sections() if section.startswith("env:")}
    if set(choices.get("options", [])) != environment_names or choices.get("default") != "factory":
        errors.append("VS Code 原生任务的环境列表与 PlatformIO 不一致")
    if len(task_labels) != 4 or any(task.get("type") != "process" for task in task_config.get("tasks", [])):
        errors.append("VS Code 编译/上传/清理/监视任务必须使用四个独立原生入口")
    for button in editor_settings.get("platformio-ide.toolbar", []):
        commands = button.get("commands")
        if isinstance(commands, list):
            for command in commands:
                if command.get("id") != "workbench.action.tasks.runTask" or not command.get("args") or command["args"][0] not in task_labels:
                    errors.append("VS Code 工具栏存在无效任务绑定")
    for filename in ("idf/CMakeLists.txt", "idf/main/CMakeLists.txt", "idf/main/idf_component.yml",
                     "idf/main/Kconfig.projbuild", "idf/sdkconfig.defaults", "idf/partitions.csv", "scripts/build_arduino.py",
                     "scripts/build_idf.py", "src/audio/test_audio.inc"):
        if not (root / filename).is_file():
            errors.append("缺少构建入口或音频资源：" + filename)
    idf_examples = re.search(r"set\(VOICE_EXAMPLES ([^)]*)\)", (root / "idf/CMakeLists.txt").read_text(encoding="utf-8"))
    if not idf_examples or set(idf_examples.group(1).split()) != expected:
        errors.append("ESP-IDF 示例列表不一致")
    if errors:
        print("\n".join(errors))
        return 1
    print("Repository checks: 8 examples, shared-layer isolation, metadata and local links PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
