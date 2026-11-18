# 固件与构建产物

目前以源码及本机构建结果交付，三种入口均不自动烧录。

| 入口 | 应用产物 | 日志 |
| --- | --- | --- |
| PlatformIO | `.pio/build/<环境名>/firmware.bin`、`firmware.elf` | `.build/logs/<环境名>.log` |
| Arduino CLI | `.build/arduino/build/<示例名>/` 下的 `.ino.bin`、`.ino.elf` 等 | `.build/logs/arduino/<示例名>.log` |
| ESP-IDF 批量脚本 | `.build/idf/<示例名>/t_keyboard_voice.bin`、`.elf`，以及配套 bootloader、分区表和烧录参数 | `.build/logs/idf/<示例名>.log` |

PlatformIO 使用环境名，如 `factory`；其余入口使用示例名，如 `Factory`。

推荐使用对应构建系统的完整上传流程：

- PlatformIO：`pio run -e factory -t upload`。
- Arduino IDE：按 [Arduino 使用说明](../docs/arduino_ide.md) 选择板型、菜单及端口后上传。
- ESP-IDF：按 [ESP-IDF 使用说明](../docs/esp_idf.md) 显式指定 `VOICE_EXAMPLE=Factory` 后执行 `build flash monitor`。

三种入口的分区方案不同；切换时不要混用旧的 bootloader、分区表与新应用。必要时先备份再擦除 Flash；擦除会丢失 Flash 中保存的数据，不是默认无损操作。

单独的 `firmware.bin`、`<示例名>.ino.bin` 或 `t_keyboard_voice.bin` 是应用镜像，不能当作从 0 地址烧录的整包。合并镜像、烧录参数与偏移须以当前入口实际生成的文件为准。

尚未完成实机验收，不将仅编译通过的二进制标记为正式出厂固件。正式发布前需记录板型/硬件版本、源码版本、工具链、测试结果、烧录偏移与 SHA-256，并完成资源许可确认。
