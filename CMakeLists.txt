# CMakeList.txt: test 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.28)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(PROJECT_NAME "chadtest")

project (${PROJECT_NAME})

if(MSVC)
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	add_compile_options("/utf-8")
else()
	# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif()

file(GLOB SOURCE_FILE
    ${CMAKE_SOURCE_DIR}/src/*.h
	${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/src/site/mteam/*.h
	${CMAKE_SOURCE_DIR}/src/site/mteam/*.cpp
    ${CMAKE_SOURCE_DIR}/utils/*.h
    ${CMAKE_SOURCE_DIR}/utils/*.cpp
)

# 将源代码添加到此项目的可执行文件。
add_executable(${PROJECT_NAME} ${SOURCE_FILE})

add_library(${PROJECT_NAME}_lib ${SOURCE_FILE})

include_directories(
	${CMAKE_SOURCE_DIR}/3rdparty/include
	${CMAKE_SOURCE_DIR}/utils
    ${CMAKE_SOURCE_DIR}/src/site
)

if(!WIN32)
    include_directories(
        /usr/include
    )
endif()

if(MSVC)
	# if use ninja
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set_target_properties(${PROJECT_NAME} PROPERTIES
            MSVC_RUNTIME_LIBRARY "MultiThreadedDebug"
        )
    else()
        set_target_properties(${PROJECT_NAME} PROPERTIES
            MSVC_RUNTIME_LIBRARY "MultiThreaded"
        )
    endif()
else()
endif()

set(FILES_TO_COPY 
    ${CMAKE_SOURCE_DIR}/src/config.toml
    ${CMAKE_SOURCE_DIR}/src/config_tv_name.toml
)

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${FILES_TO_COPY}
        $<TARGET_FILE_DIR:${PROJECT_NAME}>  # 可执行文件所在目录
)

set(WIN32_DLL
    ${CMAKE_SOURCE_DIR}/3rdparty/lib/win/MediaInfo.dll
)

if(WIN32)
    add_definitions(-DUNICODE -D_UNICODE)
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            ${WIN32_DLL}
            $<TARGET_FILE_DIR:${PROJECT_NAME}>  # 可执行文件所在目录
    )
else()

    find_library(MEDIAINFO_LIB mediainfo)
    find_library(ZEN_LIB zen)

    if (NOT MEDIAINFO_LIB OR NOT ZEN_LIB)
        message(FATAL_ERROR "MediaInfo or ZenLib not found")
    else()
        message("found MediaInfo " ${MEDIAINFO_LIB})
        message("found ZenLib " ${ZEN_LIB})
    endif()

    target_link_libraries(${PROJECT_NAME}
        ${MEDIAINFO_LIB}
        ${ZEN_LIB}
    )
endif()

# TODO: 如有需要，请添加测试并安装目标。
add_subdirectory(tests)