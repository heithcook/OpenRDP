cmake_minimum_required(VERSION 3.24)
project(OpenRDP VERSION 0.1.0 LANGUAGES CXX)

option(OPENRDP_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Widgets Network Test)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FREERDP REQUIRED IMPORTED_TARGET freerdp3 freerdp-client3 winpr3)

add_library(openrdp_core STATIC
    src/app/ConnectionHistory.cpp
    src/rdp/RdpSettings.cpp
    src/rdp/RdpInput.cpp
    src/rdp/RdpError.cpp
)
target_include_directories(openrdp_core PUBLIC src)
target_link_libraries(openrdp_core PUBLIC Qt6::Core PkgConfig::FREERDP)

add_executable(openrdp
    src/main.cpp
    src/app/Application.cpp
    src/gui/MainWindow.cpp
    src/gui/CredentialDialog.cpp
    src/gui/CertificateDialog.cpp
    src/gui/RdpDisplayWidget.cpp
    src/rdp/RdpSession.cpp
    src/rdp/RdpRenderer.cpp
)
target_include_directories(openrdp PRIVATE src)
target_link_libraries(openrdp PRIVATE openrdp_core Qt6::Gui Qt6::Widgets Qt6::Network)

foreach(target openrdp openrdp_core)
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wformat=2 -Wformat-security)
endforeach()

if(OPENRDP_ENABLE_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    foreach(target openrdp openrdp_core)
        target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    endforeach()
    target_link_options(openrdp PRIVATE -fsanitize=address,undefined)
    target_link_options(openrdp_core INTERFACE -fsanitize=address,undefined)
endif()

enable_testing()
add_subdirectory(tests)
