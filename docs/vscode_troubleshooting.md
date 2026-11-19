# VS Code / PlatformIO Build Entry and Troubleshooting

[English](#english) | [简体中文](#简体中文)

## English

### `Cannot read properties of undefined (reading 'id')`

Local logs traced this exception to `TaskManager.runTask` in PlatformIO IDE **3.3.4**, along with `platformio-ide.build already exists`. The extension filters tasks by the selected environment. If tasks are not ready or the cache and environment disagree, it still reads an absent task's `id`, raising a JavaScript exception before invoking the compiler. This is not a C++ syntax diagnostic.

The project supplies local alternative entries without modifying VS Code/PlatformIO installation files or clearing global extension data:

- `.vscode/tasks.json`: native `process` tasks that invoke PlatformIO Core directly.
- `.vscode/settings.json`: project-local build, upload, clean, and monitor status-bar buttons bound to those tasks, with independent Home/terminal entries retained.
- Each operation selects one of the current 8 environments, defaulting to `factory`, independently of the extension's cached environment selection.
- `Ctrl+Shift+B` runs `T-Keyboard Voice: Build` by default.

These entries bypass the failing extension task-selection path; **they do not claim to fix every extension defect**. The original command-palette `PlatformIO: Build/Upload` commands still belong to the extension and are not the project tasks below.

### Usage

1. Open the **`T-Keyboard-Voice` repository root** in VS Code, not merely one file or a different old project directory.
2. Save edits, then run **Developer: Reload Window** so the window reloads project settings, buttons, and tasks. Avoid repeatedly opening more windows to rebuild the index.
3. Press **Ctrl+Shift+B** or the build check mark, then choose `factory` or another environment. The terminal should execute a direct `platformio run ... --environment <environment>` command and report `[SUCCESS]` when complete.
4. Click the arrow only to upload, or use **Tasks: Run Task → T-Keyboard Voice: Upload**. Upload writes to the device: connect the correct board and close programs occupying its port first. This fix did not automatically upload firmware.
5. The monitor button runs **T-Keyboard Voice: Monitor** at 115200 baud. The clean button runs **T-Keyboard Voice: Clean**, removing only the selected environment's build products.

The picker contains `factory`, `power`, `display`, `keys`, `ws2812`, `audio_playback`, `audio_record`, and `qwiic`. It does not read the original extension's status-bar environment cache; the new picker determines the environment actually used for that operation.

### PlatformIO Core Path

Windows tasks use VS Code variables to form the default built-in Core path:

```text
${env:USERPROFILE}/.platformio/penv/Scripts/platformio.exe
```

This executable was verified locally as PlatformIO Core 6.1.19. Other operating systems default to `platformio` on PATH. If Core is installed elsewhere, or only through pip without that built-in environment, change the corresponding `command` in `.vscode/tasks.json` to the actual executable. Do not substitute ESP-IDF's `idf.py`.

You can also test directly from the repository root without the IDE:

```bash
pio run -e factory
```

The native command built successfully, and IDE metadata was verified to contain the current 8 environments and nonempty targets. The GUI was not automated, so **buttons must still be clicked in the actual window after reloading**. Command-line success is not manual verification of every GUI action.

### `.ino.cpp: No such file or directory`

The old source-selection approach let PlatformIO create and clean up temporary `.ino.cpp` files inside example directories. Concurrent builds and IDE metadata/index scans could remove the same temporary paths.

`scripts/select_example.py` now generates a stable C++ entry per environment in `.pio/build/<environment>/application/`, using the selected original application and shared drivers without creating temporary `.ino.cpp` files in `examples/`. Files are updated atomically only when their content changes and are not removed by metadata scanning. All 8 PlatformIO environments passed regression builds using this entry.

This build issue is handled separately from `reading 'id'`; the two are not assumed to be the same exception without evidence.

### If Problems Remain

- Confirm that the window opens the current repository and uses the native project tasks above, not another window or an old extension command.
- Let initial indexing/environment loading finish before operating the controls. If using the official task tree, refresh it and reselect a valid environment first.
- If native tasks build correctly but old buttons still raise JavaScript exceptions, retain Extension Host logs and PlatformIO output. Do not rewrite hardware drivers or delete all SDK caches for that reason.
- Missing Core, serial ports, or USB devices require their own troubleshooting; they are not the same as an absent task `id`.

---

## 简体中文

**VS Code / PlatformIO 编译入口与报错排查**

### `Cannot read properties of undefined (reading 'id')`

本机日志已定位到 PlatformIO IDE **3.3.4** 扩展的 `TaskManager.runTask`，并出现过 `platformio-ide.build already exists`。扩展通过当前选中环境过滤任务列表，列表未就绪或缓存与环境不一致时，仍读取空任务的 `id`，因此在真正调用编译器前抛出 JavaScript 异常。这不是 C++ 语法诊断。

本项目提供局部替代入口，不修改 VS Code/PlatformIO 扩展的安装文件，也不清空全局扩展数据：

- `.vscode/tasks.json`：原生 `process` 任务，直接调用 PlatformIO Core。
- `.vscode/settings.json`：将本项目状态栏的编译、上传、清理和串口按钮绑定到这些原生任务，保留独立的 Home/终端入口。
- 每次操作从当前 8 个有效环境中选择一个，默认 `factory`，不依赖扩展缓存的选中环境。
- `Ctrl+Shift+B` 默认执行 `T-Keyboard Voice: Build`。

这些入口绕开了发生异常的扩展任务选择路径；**没有声称修复了扩展自身的所有问题**。命令面板里的原始 `PlatformIO: Build/Upload` 仍属于扩展入口，不等同于下列项目任务。

### 使用步骤

1. 在 VS Code 中打开 **`T-Keyboard-Voice` 仓库根目录**，不要只打开某个源文件，也不要误用另一个旧工程目录。
2. 保存未保存的编辑，执行命令面板中的 **Developer: Reload Window**，让本窗口重新读取项目设置、按钮和任务；不要反复启动多个窗口重建索引。
3. 按 **Ctrl+Shift+B** 或点击状态栏的编译勾号，选择 `factory` 或其他所需环境。终端应执行直接的 `platformio run ... --environment <环境>`，成功时显示 `[SUCCESS]`。
4. 上传时才点击箭头，或运行 **Tasks: Run Task → T-Keyboard Voice: Upload**。上传会写入设备，需先连接正确板卡、关闭占用串口的程序；本次修复没有自动上传。
5. 串口按钮对应 **T-Keyboard Voice: Monitor**，115200 波特率；清理按钮对应 **T-Keyboard Voice: Clean**，只清理选定环境的编译产物。

环境选择列表为：`factory`、`power`、`display`、`keys`、`ws2812`、`audio_playback`、`audio_record`、`qwiic`。它不读取原扩展状态栏的环境缓存；每次弹出的环境选择才是该任务实际使用的值。

### PlatformIO Core 路径

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

### `.ino.cpp: No such file or directory`

旧的源码选择方式让 PlatformIO 在示例目录生成并清理临时 `.ino.cpp`。构建和 IDE 元数据/索引扫描同时运行时，存在互相清理同一路径的风险。

`scripts/select_example.py` 现在为每个环境在自己的 `.pio/build/<环境>/application/` 中生成稳定 C++ 入口，直接使用所选的原应用和共用驱动，不在 `examples/` 内生成临时 `.ino.cpp`。文件只在内容变化时原子更新，元数据扫描不会负责删除它。全部 8 个 PlatformIO 环境已在此入口下回归通过。

这是与 `reading 'id'` 分别处理的构建问题；不将两者未经验证地当作同一异常。

### 仍有问题时

- 检查窗口确实打开当前仓库，并使用上述项目原生任务，而非其他窗口或原扩展的旧命令。
- 等待初次索引/环境加载结束，再操作。仍用官方任务树时，先刷新任务并重新选择有效环境。
- 原生任务若能正常编译，而旧按钮仍报 JavaScript 异常，保留 VS Code 的 Extension Host 日志和 PlatformIO 输出，不要为此改写硬件驱动或删除所有 SDK 缓存。
- 若提示找不到 Core、串口或 USB 设备，则按相应错误处理；这些不等同于空任务 `id` 异常。
