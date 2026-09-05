set(_UMURMUR_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../third_party/umurmur")
set(UMURMUR_SRC "${_UMURMUR_ROOT}/src")

execute_process(
    COMMAND "${CMAKE_CURRENT_LIST_DIR}/../../scripts/apply-umurmur-patches.sh"
    RESULT_VARIABLE _UMURMUR_PATCH_RC
    OUTPUT_VARIABLE _UMURMUR_PATCH_OUT
    ERROR_VARIABLE _UMURMUR_PATCH_ERR
    WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../.."
)
if(NOT _UMURMUR_PATCH_RC EQUAL 0)
    message(FATAL_ERROR
        "umurmur patch apply failed (rc=${_UMURMUR_PATCH_RC})\n"
        "${_UMURMUR_PATCH_OUT}${_UMURMUR_PATCH_ERR}")
endif()

idf_component_register(
    SRCS
        "${UMURMUR_SRC}/Mumble.pb-c.c"
        "${UMURMUR_SRC}/ban.c"
        "${UMURMUR_SRC}/channel.c"
        "${UMURMUR_SRC}/client.c"
        "${UMURMUR_SRC}/crypt.c"
        "${UMURMUR_SRC}/memory.c"
        "${UMURMUR_SRC}/messagehandler.c"
        "${UMURMUR_SRC}/messages.c"
        "${UMURMUR_SRC}/pds.c"
        "${UMURMUR_SRC}/server.c"
        "${UMURMUR_SRC}/ssli_mbedtls.c"
        "${UMURMUR_SRC}/timer.c"
        "${UMURMUR_SRC}/util.c"
        "${UMURMUR_SRC}/voicetarget.c"
        "conf_esp.c"
        "log_esp.c"
        "umurmur_task.c"
    INCLUDE_DIRS
        "."
        "include_override"
        "${UMURMUR_SRC}"
    REQUIRES
        mbedtls
        protobuf-c
        esp_timer
        heap
        esp_common
        freertos
        lwip
        vfs
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE
    _GNU_SOURCE
)

target_include_directories(${COMPONENT_LIB} BEFORE PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/include_override"
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=format)

target_compile_options(${COMPONENT_LIB} PRIVATE -include "${CMAKE_CURRENT_LIST_DIR}/include_override/sharedmemory_force.h")
