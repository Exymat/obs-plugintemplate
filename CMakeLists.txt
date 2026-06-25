cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" OFF)
option(ENABLE_QT "Use Qt functionality" OFF)

include(compilerconfig)
include(defaults)
include(helpers)

include(FetchContent)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
FetchContent_Declare(
  lunasvg
  GIT_REPOSITORY https://github.com/sammycage/lunasvg.git
  GIT_TAG v3.5.0
)
set(LUNASVG_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(lunasvg)

# LunaSVG/plutovg trigger warnings; OBS CI uses warnings-as-errors. Use compiler-specific flags.
if(TARGET plutovg)
  if(MSVC)
    target_compile_options(plutovg PRIVATE /WX- /wd4100 /wd4244 /wd4267)
  else()
    target_compile_options(plutovg PRIVATE -Wno-error -Wno-unused-parameter -Wno-sign-compare)
  endif()
endif()
if(TARGET lunasvg)
  if(MSVC)
    target_compile_options(lunasvg PRIVATE /WX- /wd4100 /wd4189 /wd4244 /wd4267)
  else()
    target_compile_options(lunasvg PRIVATE -Wno-error -Wno-ignored-qualifiers -Wno-unused-parameter
                                       -Wno-sign-compare)
  endif()
endif()

add_library(${CMAKE_PROJECT_NAME} MODULE)

find_package(libobs REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs lunasvg::lunasvg)

if(ENABLE_FRONTEND_API)
  find_package(obs-frontend-api REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
endif()

if(ENABLE_QT)
  find_package(Qt6 COMPONENTS Widgets Core)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets)
  target_compile_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
  )
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
  )
endif()

target_sources(
  ${CMAKE_PROJECT_NAME}
  PRIVATE
    src/plugin-main.cpp
    src/pill-source.cpp
    src/pill-animation.cpp
    src/svg-text.cpp
    src/pill-layout.cpp
    src/time-format.cpp
)

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})
