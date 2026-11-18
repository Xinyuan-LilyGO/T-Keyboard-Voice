get_filename_component(VOICE_REPO "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
if(NOT DEFINED VOICE_DEPS_DIR)
    set(VOICE_DEPS_DIR "${VOICE_REPO}/.build/deps")
endif()
set(lib "${VOICE_DEPS_DIR}/ButtonSense/src")
idf_component_register(SRCS "${lib}/ButtonSense.cpp" "${lib}/ButtonManager.cpp"
                       INCLUDE_DIRS "${lib}" REQUIRES espressif__arduino-esp32)
