cmake_minimum_required(VERSION 3.12)

project(sokit LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)
find_package(Qt6LinguistTools)

file(GLOB SRC_FILES "src/sokit/*.cpp" "src/sokit/*.qrc" "src/sokit/*.rc" "src/sokit/*.ui")
file(GLOB HEAD_FILES "src/sokit/*.h")

if(CMAKE_HOST_WIN32)
	enable_language("RC")
	if(CMAKE_BUILD_TYPE STREQUAL "Release")
		add_executable(${PROJECT_NAME} WIN32 ${SRC_FILES} ${HEAD_FILES} )
	else()
		add_executable(${PROJECT_NAME} ${SRC_FILES} ${HEAD_FILES} )
	endif()
else()
	add_executable(${PROJECT_NAME} ${SRC_FILES} ${HEAD_FILES} "qml.qrc" )
endif()

qt_add_translations(${PROJECT_NAME} TS_FILES "src/sokit/sokit.ts")

target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets Qt6::Network )

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/)