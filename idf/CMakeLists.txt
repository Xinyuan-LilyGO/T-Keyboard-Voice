cmake_minimum_required(VERSION 3.16)
set(PROJECT_VER "0.1.0")
# 组件管理器解压的源码没有独立 Git 元数据，不应误用上层应用仓库的 HEAD。
# 这也允许从 ZIP 或尚无首次提交的仓库构建，且不需要创建提交。
set(ENV{GIT_CEILING_DIRECTORIES} "${CMAKE_CURRENT_LIST_DIR}")

# 选择现有示例，不另外复制一套应用和驱动。
set(VOICE_EXAMPLE "Factory" CACHE STRING "T-Keyboard Voice example")
set(VOICE_EXAMPLES Power Display Keys WS2812 AudioPlayback AudioRecord QWIIC Factory)
set_property(CACHE VOICE_EXAMPLE PROPERTY STRINGS ${VOICE_EXAMPLES})
# ESP-IDF 的依赖预扫描在独立 CMake 脚本中运行，不自动继承自定义缓存变量。
set(ENV{TKEYBOARD_VOICE_EXAMPLE} "${VOICE_EXAMPLE}")
if(NOT VOICE_EXAMPLE IN_LIST VOICE_EXAMPLES)
    message(FATAL_ERROR "Unknown VOICE_EXAMPLE: ${VOICE_EXAMPLE}")
endif()
get_filename_component(VOICE_REPO "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(VOICE_DEPS_DIR "${VOICE_REPO}/.build/deps" CACHE PATH "Prepared Arduino library dependencies")
foreach(dependency SensorLib LovyanGFX ButtonSense)
    if(NOT EXISTS "${VOICE_DEPS_DIR}/${dependency}/src")
        message(FATAL_ERROR "Missing ${dependency}; run python scripts/prepare_dependencies.py from the repository")
    endif()
endforeach()

# 核心和应用必须使用一致的 USB CDC、PSRAM 与 SensorLib 配置。
add_compile_definitions(ARDUINO_USB_MODE=1 ARDUINO_USB_CDC_ON_BOOT=1 BOARD_HAS_PSRAM
                        SENSORLIB_EXCLUDE_ALL=1 SENSORLIB_EXCLUDE_PMIC=0 SENSORLIB_EXCLUDE_PMIC_AXP2101=0)
set(SDKCONFIG_DEFAULTS "${CMAKE_CURRENT_LIST_DIR}/sdkconfig.defaults")
set(COMPONENTS main)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(t_keyboard_voice)
