get_filename_component(VOICE_REPO "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
if(NOT DEFINED VOICE_DEPS_DIR)
    set(VOICE_DEPS_DIR "${VOICE_REPO}/.build/deps")
endif()
set(lib "${VOICE_DEPS_DIR}/LovyanGFX/src")
file(GLOB sources
    "${lib}/lgfx/Fonts/efont/*.c" "${lib}/lgfx/Fonts/IPA/*.c" "${lib}/lgfx/Fonts/lvgl/*.c"
    "${lib}/lgfx/utility/*.c" "${lib}/lgfx/v1/*.cpp" "${lib}/lgfx/v1/lv_font/*.c"
    "${lib}/lgfx/v1/misc/*.cpp" "${lib}/lgfx/v1/panel/*.cpp" "${lib}/lgfx/v1/touch/*.cpp"
    "${lib}/lgfx/v1/platforms/arduino_default/*.cpp" "${lib}/lgfx/v1/platforms/esp32/*.cpp"
    "${lib}/lgfx/v1/platforms/esp32c3/*.cpp" "${lib}/lgfx/v1/platforms/esp32s2/*.cpp"
    "${lib}/lgfx/v1/platforms/esp32s3/*.cpp" "${lib}/lgfx/v1/platforms/esp32p4/*.cpp")
idf_component_register(SRCS ${sources} INCLUDE_DIRS "${lib}"
                       REQUIRES espressif__arduino-esp32 nvs_flash efuse esp_lcd driver esp_timer esp_mm)
