# 主机回归测试

从仓库目录执行：

```bash
python scripts/test_host.py
```

- `es7210_samples_test.cpp`：线序、MIC 通道隔离、有符号边界和无效通道。
- `power_key_standby_test.cpp`：启动释放、去抖、长按、一次性事件和 millis 回绕。

脚本自动使用 GCC/Clang；Windows 优先使用原生 MSVC，支持自动加载 Build Tools 环境。可设置 `CXX` 为原生编译器的完整路径。不要使用 ESP32 交叉编译器运行主机测试。

输出在 `.build/host/`，不进入版本控制。这些测试不访问硬件，不证明 I2S 电气信号或休眠功耗合格。
