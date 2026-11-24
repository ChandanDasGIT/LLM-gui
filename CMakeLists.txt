cmake_minimum_required(VERSION 3.21)

project(LLMgui VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

# ---------------------------------------------------------------- the backend (no Qt)
# Everything under backend/ is plain C++20. It is built as its own library and does not link to Qt,
# so a Qt include in there fails to compile.
file(GLOB_RECURSE BACKEND_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/backend/*.cpp")

add_library(llmbackend STATIC ${BACKEND_SOURCES})
target_include_directories(llmbackend PUBLIC "${CMAKE_CURRENT_LIST_DIR}/backend")
target_compile_options(llmbackend PRIVATE -Wall -Wextra)

# ---------------------------------------------------------------- the backend tests (no Qt)
# tests/backend/.../abc_test.cpp is found by itself, and ctest gets one entry named abc.
enable_testing()

file(GLOB_RECURSE BACKEND_TEST_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/tests/backend/*_test.cpp")

add_executable(backendtests tests/backendtests.cpp ${BACKEND_TEST_SOURCES})
target_include_directories(backendtests PRIVATE "${CMAKE_CURRENT_LIST_DIR}/tests")
target_compile_options(backendtests PRIVATE -Wall -Wextra)
target_compile_definitions(backendtests PRIVATE LLMGUI_SOURCE_DIR="${CMAKE_CURRENT_LIST_DIR}")
target_link_libraries(backendtests PRIVATE llmbackend)

foreach(file ${BACKEND_TEST_SOURCES})
    get_filename_component(stem "${file}" NAME_WE)
    string(REGEX REPLACE "_test$" "" suite "${stem}")
    add_test(NAME ${suite} COMMAND backendtests ${suite})
endforeach()

find_package(Qt6 REQUIRED COMPONENTS Quick Network Concurrent)
find_package(Qt6 QUIET COMPONENTS Test)
qt_standard_project_setup(REQUIRES 6.10)

# wrapper/  : QObject classes that connect the backend to QML (any subfolders)
file(GLOB_RECURSE WRAPPER_SOURCES CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_LIST_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/wrapper/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/wrapper/*.h")

# Qt's generated registration code includes the wrapper headers by bare file name,
# so every folder under wrapper/ has to be on the include path.
set(WRAPPER_DIRS "")
foreach(f ${WRAPPER_SOURCES})
    get_filename_component(d "${CMAKE_CURRENT_LIST_DIR}/${f}" DIRECTORY)
    list(APPEND WRAPPER_DIRS "${d}")
endforeach()
list(REMOVE_DUPLICATES WRAPPER_DIRS)

# ---------------------------------------------------------------- the app
# frontend/ : the QML files (any subfolders). File names must start with a capital letter and be
#             unique across all subfolders.
file(GLOB_RECURSE FRONTEND_QML CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_LIST_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/frontend/*.qml")

set_source_files_properties(frontend/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_executable(appLLM-gui
    main.cpp
)

qt_add_qml_module(appLLM-gui
    URI LLMgui
    QML_FILES
        Main.qml
        ${FRONTEND_QML}
    SOURCES
        ${WRAPPER_SOURCES}
)

target_include_directories(appLLM-gui PRIVATE "${CMAKE_CURRENT_LIST_DIR}" ${WRAPPER_DIRS})
target_compile_options(appLLM-gui PRIVATE -Wall -Wextra)
target_link_libraries(appLLM-gui PRIVATE Qt6::Quick Qt6::Network Qt6::Concurrent llmbackend)

# The recommended models (":/resources/catalogue.json").
qt_add_resources(appLLM-gui catalogue PREFIX "/" FILES resources/catalogue.json)

# ---------------------------------------------------------------- the wrapper tests (Qt, fake Ollama)
# tests/wrapper/abc_qtcheck.cpp is found by itself, and ctest gets one entry named qtabc.
if(TARGET Qt6::Test)
    file(GLOB WRAPPER_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/tests/wrapper/*_qtcheck.cpp")
    add_executable(wrappertests tests/backendtests.cpp tests/wrapper/fakeollama.cpp tests/wrapper/fakeollama.h
                   ${WRAPPER_TEST_SOURCES} ${WRAPPER_SOURCES})
    target_include_directories(wrappertests PRIVATE "${CMAKE_CURRENT_LIST_DIR}" "${CMAKE_CURRENT_LIST_DIR}/tests"
                               "${CMAKE_CURRENT_LIST_DIR}/tests/wrapper" ${WRAPPER_DIRS})
    target_compile_options(wrappertests PRIVATE -Wall -Wextra)
    target_link_libraries(wrappertests PRIVATE Qt6::Qml Qt6::Network Qt6::Concurrent Qt6::Test llmbackend)
    qt_add_resources(wrappertests catalogue_tests PREFIX "/" FILES resources/catalogue.json)
    foreach(file ${WRAPPER_TEST_SOURCES})
        get_filename_component(stem "${file}" NAME_WE)
        string(REGEX REPLACE "_qtcheck$" "" suite "${stem}")
        add_test(NAME qt${suite} COMMAND wrappertests qt${suite})
        set_tests_properties(qt${suite} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    endforeach()
endif()
