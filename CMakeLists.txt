cmake_minimum_required(VERSION 3.20)
project(WindowsRepairTool LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)
option(WRT_SANITIZERS "Enable address/undefined-behavior sanitizers for tests" OFF)

add_library(wrt_core INTERFACE)
target_compile_features(wrt_core INTERFACE cxx_std_17)
target_include_directories(wrt_core INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/Shared")

if(BUILD_TESTING)
    add_executable(wrt_tests tests/unit_tests.cpp)
    target_link_libraries(wrt_tests PRIVATE wrt_core)
    if(MSVC)
        target_compile_options(wrt_tests PRIVATE /W4 /WX /utf-8)
    else()
        target_compile_options(wrt_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
        if(WRT_SANITIZERS)
            target_compile_options(wrt_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
            target_link_options(wrt_tests PRIVATE -fsanitize=address,undefined)
        endif()
    endif()
    foreach(case IN ITEMS input input_invalid verbosity padding language quoting versions
        versions_invalid output_bounds output_utf16 output_split output_malformed repair_modes repair_risks
        result_status assets asset_rejection update_gate transfer transfer_failure installer_arguments installer_exit)
        add_test(NAME unit.${case} COMMAND wrt_tests ${case})
    endforeach()
endif()

if(WIN32)
    enable_language(RC)
    add_library(wrt_windows INTERFACE)
    target_link_libraries(wrt_windows INTERFACE wrt_core shell32 ole32 uuid)
    target_compile_definitions(wrt_windows INTERFACE UNICODE _UNICODE NOMINMAX _WIN32_WINNT=0x0603)

    set(repair_resources "${CMAKE_CURRENT_SOURCE_DIR}/Windows Reparatur/Windows Reparatur.rc")
    set(launcher_resources "${CMAKE_CURRENT_SOURCE_DIR}/WRTLauncher/WRTLauncher.rc")
    if(MINGW)
        # windres' C preprocessor cannot read the existing UTF-16LE RC source.
        # Convert only the build copy; keep Visual Studio's resource untouched.
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${repair_resources}" "${launcher_resources}")
        file(STRINGS "${repair_resources}" resource_lines ENCODING UTF-16LE)
        list(JOIN resource_lines "\n" resource_text)
        string(REPLACE "..\\\\wrench.ico" "${CMAKE_CURRENT_SOURCE_DIR}/wrench.ico" resource_text "${resource_text}")
        set(repair_resources "${CMAKE_CURRENT_BINARY_DIR}/repair_resources.rc")
        file(WRITE "${repair_resources}" "${resource_text}\n")
        file(READ "${launcher_resources}" resource_text)
        string(REPLACE "..\\\\wrench.ico" "${CMAKE_CURRENT_SOURCE_DIR}/wrench.ico" resource_text "${resource_text}")
        set(launcher_resources "${CMAKE_CURRENT_BINARY_DIR}/launcher_resources.rc")
        file(WRITE "${launcher_resources}" "${resource_text}")
    endif()
    add_executable(WRT "Windows Reparatur/Windows Reparatur.cpp"
        "Windows Reparatur/localization.cpp" "${repair_resources}" Shared/admin.rc)
    target_include_directories(WRT PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Windows Reparatur")
    add_executable(WRTLauncher WRTLauncher/WRTLauncher.cpp WRTLauncher/localization.cpp
        "${launcher_resources}" Shared/admin.rc)
    target_include_directories(WRTLauncher PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/WRTLauncher")
    target_link_libraries(WRT PRIVATE wrt_windows)
    target_link_libraries(WRTLauncher PRIVATE wrt_windows winhttp wintrust crypt32 advapi32)
    foreach(app IN ITEMS WRT WRTLauncher)
        target_include_directories(${app} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Shared")
        set_target_properties(${app} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/package/$<CONFIG>/Binaries")
        if(MSVC)
            target_compile_options(${app} PRIVATE /W4 /utf-8 /permissive-)
            target_link_options(${app} PRIVATE /MANIFEST:NO)
        endif()
    endforeach()
    add_library(wrt_german MODULE "German Locale/German Locale.rc" "German Locale/dllmain.cpp")
    set_target_properties(wrt_german PROPERTIES PREFIX "" OUTPUT_NAME de
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/package/$<CONFIG>/locale")
    foreach(script IN ITEMS "Simple repair.bat" "Default repair.bat" "Extended repair.bat")
        add_custom_command(TARGET WRT POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory
            "${CMAKE_BINARY_DIR}/package/$<CONFIG>/Batch scripts"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/Batch scripts/${script}"
            "${CMAKE_BINARY_DIR}/package/$<CONFIG>/Batch scripts/${script}")
    endforeach()

    if(BUILD_TESTING)
        add_executable(wrt_process_fixture tests/process_fixture.cpp)
        target_link_libraries(wrt_process_fixture PRIVATE wrt_windows)
        add_library(wrt_resource_fixture MODULE tests/resource_fixture.cpp tests/resource_fixture.rc)
        set_target_properties(wrt_resource_fixture PROPERTIES PREFIX "" OUTPUT_NAME en
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test_resources/$<CONFIG>/locale")
        add_executable(wrt_windows_tests tests/windows_tests.cpp tests/windows_tests.rc)
        target_link_libraries(wrt_windows_tests PRIVATE wrt_windows winhttp wintrust crypt32 advapi32)
        add_dependencies(wrt_windows_tests wrt_process_fixture wrt_resource_fixture)
        if(MSVC)
            target_compile_options(wrt_windows_tests PRIVATE /W4 /WX /utf-8)
        endif()
        add_test(NAME windows.process_and_resources COMMAND wrt_windows_tests
            "$<TARGET_FILE:wrt_process_fixture>" "${CMAKE_BINARY_DIR}/test_resources/$<CONFIG>")
        set_tests_properties(windows.process_and_resources PROPERTIES TIMEOUT 60)
    endif()
endif()
