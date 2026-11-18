# 只编译本板需要的 AXP2101 及共用通信实现，不引入其他传感器驱动。
get_filename_component(VOICE_REPO "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
if(NOT DEFINED VOICE_DEPS_DIR)
    set(VOICE_DEPS_DIR "${VOICE_REPO}/.build/deps")
endif()
set(lib "${VOICE_DEPS_DIR}/SensorLib")
file(GLOB axp_sources "${lib}/src/pmic/xpowers/axp2101/*.cpp")
idf_component_register(SRCS ${axp_sources} "${lib}/src/SensorLibExceptionFix.cpp" "${lib}/src/platform/SensorCommStatic.cpp"
                       INCLUDE_DIRS "${lib}/src" REQUIRES espressif__arduino-esp32)
