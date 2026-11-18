# VS Code / PlatformIO 编译入口与报错排查

## `Cannot read properties of undefined (reading 'id')`

本机日志已定位到 PlatformIO IDE **3.3.4** 扩展的 `TaskManager.runTask`，并出现过 `platformio-ide.build already exists`。扩展通过当前选中环境过滤任务列表，列表未就绪或缓存与环境不一致时，仍读取空任务的 `id`，因此在真正调用编译器前抛出 JavaScript 异常。这不是 C++ 语法诊断。

本项目提供局部替代入口，不修改 VS Code/PlatformIO 扩展的安装文件，也不清空全局扩展数据：

- `.vscode/tasks.json`：原生 `process` 任务，直接调用 PlatformIO Core。
- `.vscode/settings.json`：将本项目状态栏的编译、上传、清理和串口按钮绑定到这些原生任务，保留独立的 Home/终端入口。
- 每次操作从当前 8 个有效环境中选择一个，默认 `factory`，不依赖扩展缓存的选中环境。
- `Ctrl+Shift+B` 默认执行 `T-Keyboard Voice: Build`。

这些入口绕开了发生异常的扩展任务选择路径；**没有声称修复了扩展自身的所有问题**。命令面板里的原始 `PlatformIO: Build/Upload` 仍属于扩展入口，不等同于下列项目任务。

## 使用步骤

1. 在 VS Code 中打开 **`T-Keyboard-Voice` 仓库根目录**，不要只打开某个源文件，也不要误用另一个旧工程目录。
2. 保存未保存的编辑，执行命令面板中的 **Developer: Reload Window**，让本窗口重新读取项目设置、按钮和任务；不要反复启动多个窗口重建索引。
3. 按 **Ctrl+Shift+B** 或点击状态栏的编译勾号，选择 `factory` 或其他所需环境。终端应执行直接的 `platformio run ... --environment <环境>`，成功时显示 `[SUCCESS]`。
4. 上传时才点击箭头，或运行 **Tasks: Run Task → T-Keyboard Voice: Upload**。上传会写入设备，需先连接正确板卡、关闭占用串口的程序；本次修复没有自动上传。
5. 串口按钮对应 **T-Keyboard Voice: Monitor**，115200 波特率；清理按钮对应 **T-Keyboard Voice: Clean**，只清理选定环境的编译产物。

环境选择列表为：`factory`、`power`、`display`、`keys`、`ws2812`、`audio_playback`、`audio_record`、`qwiic`。它不读取原扩展状态栏的环境缓存；每次弹出的环境选择才是该任务实际使用的值。

## PlatformIO Core 路径

Windows 任务使用 VS Code 变量组成默认内置 Core 路径：

```text
${env:USERPROFILE}/.platformio/penv/Scripts/platformio.exe
```

本机已验证该文件存在且为 PlatformIO Core 6.1.19。非 Windows 默认使用 PATH 中的 `platformio`。如果安装在自定义位置，或只通过 pip 安装而没有这个内置环境，请将 `.vscode/tasks.json` 中对应的 `command` 改为实际可执行文件；不要替换成 ESP-IDF 的 `idf.py`。

也可不经过 IDE，直接在仓库根目录验证：

```bash
pio run -e factory
```

原生命令已构建通过，IDE 构建元数据也已确认有当前 8 个环境及非空任务目标。由于未自动操作 GUI，**重载后的按钮仍需在实际窗口点击确认**；不能把命令行通过等同于已人工验证所有 GUI 操作。

## `.ino.cpp: No such file or directory`

旧的源码选择方式让 PlatformIO 在示例目录生成并清理临时 `.ino.cpp`。构建和 IDE 元数据/索引扫描同时运行时，存在互相清理同一路径的风险。

`scripts/select_example.py` 现在为每个环境在自己的 `.pio/build/<环境>/application/` 中生成稳定 C++ 入口，直接使用所选的原应用和共用驱动，不在 `examples/` 内生成临时 `.ino.cpp`。文件只在内容变化时原子更新，元数据扫描不会负责删除它。全部 8 个 PlatformIO 环境已在此入口下回归通过。

这是与 `reading 'id'` 分别处理的构建问题；不将两者未经验证地当作同一异常。

## 仍有问题时

- 检查窗口确实打开当前仓库，并使用上述项目原生任务，而非其他窗口或原扩展的旧命令。
- 等待初次索引/环境加载结束，再操作。仍用官方任务树时，先刷新任务并重新选择有效环境。
- 原生任务若能正常编译，而旧按钮仍报 JavaScript 异常，保留 VS Code 的 Extension Host 日志和 PlatformIO 输出，不要为此改写硬件驱动或删除所有 SDK 缓存。
- 若提示找不到 Core、串口或 USB 设备，则按相应错误处理；这些不等同于空任务 `id` 异常。
