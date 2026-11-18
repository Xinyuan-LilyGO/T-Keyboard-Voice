"""按环境选择一个示例，并且只编译它需要的公共模块。"""
Import("env")

import os

MODULES = {
    "Power": [],
    "Display": ["VoiceDisplay.cpp"],
    "Keys": ["VoiceInput.cpp"],
    "WS2812": ["VoiceRGB.cpp"],
    "AudioPlayback": ["VoiceAudio.cpp", "VoiceTestAudio.cpp"],
    "AudioRecord": ["VoiceAudio.cpp"],
    "QWIIC": [],
    "Factory": ["VoiceDisplay.cpp", "VoiceInput.cpp", "VoiceRGB.cpp", "VoiceAudio.cpp", "VoiceTestAudio.cpp"],
}

project = env.subst("$PROJECT_DIR")
example = env.GetProjectOption("custom_example")
if example not in MODULES:
    raise ValueError("未知示例：" + example)

# 不在示例目录生成临时 .ino.cpp，避免编译与 IDE 元数据扫描互相清理文件。
# 每个环境只生成一份 C++ 入口，包含它所选择的应用；硬件驱动仍独立编译。
application = os.path.join(env.subst("$BUILD_DIR"), "application")
os.makedirs(application, exist_ok=True)
entry_name = "Factory.cpp" if example == "Factory" else example + ".ino"
entry_path = os.path.join(project, "examples", example, entry_name).replace("\\", "/")
application_headers = ["VoicePower.h"] + [name.replace(".cpp", ".h") for name in MODULES[example]]
wrapper = "#include <Arduino.h>\n" + "".join('#include "' + header + '"\n' for header in application_headers)
wrapper += '#include "' + entry_path + '"\n'
wrapper_path = os.path.join(application, "main.cpp")
previous = None
if os.path.isfile(wrapper_path):
    with open(wrapper_path, encoding="utf-8") as stream:
        previous = stream.read()
if previous != wrapper:
    temporary = wrapper_path + "." + str(os.getpid()) + ".tmp"
    with open(temporary, "w", encoding="utf-8", newline="\n") as stream:
        stream.write(wrapper)
    os.replace(temporary, wrapper_path)
env.Replace(PROJECT_SRC_DIR=application)
shared = os.path.join(project, "src")
framework = env.PioPlatform().get_package_dir("framework-arduinoespressif32")
framework_libraries = [
    os.path.join(framework, "libraries", name, "src")
    for name in ("Wire", "SPI", "ESP_I2S")
]
libdeps = env.subst("$PROJECT_LIBDEPS_DIR/$PIOENV")
dependency_includes = []
if os.path.isdir(libdeps):
    dependency_includes = [
        os.path.join(libdeps, name, "src")
        for name in sorted(os.listdir(libdeps))
        if os.path.isdir(os.path.join(libdeps, name, "src"))
    ]
env.Append(CPPPATH=[shared] + framework_libraries + dependency_includes)
env.BuildSources(
    os.path.join(env.subst("$BUILD_DIR"), "shared"),
    shared,
    ["+<VoicePower.cpp>"] + ["+<" + name + ">" for name in MODULES[example]],
)
