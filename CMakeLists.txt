cmake_minimum_required(VERSION 3.1)
project(lua-etf)

option(BUILD_SHARED_LIBS "Build modules as shared libraries" ON)

find_package(PkgConfig)
include(FindPackageHandleStandardArgs)

if(LUA_VERSION)
  find_package(Lua ${LUA_VERSION} EXACT REQUIRED)
else()
  find_package(Lua REQUIRED)
  set(LUA_VERSION "${LUA_VERSION_MAJOR}.${LUA_VERSION_MINOR}" CACHE STRING "Lua version to use")
endif()

set(CMODULE_INSTALL_LIB_DIR "${CMAKE_INSTALL_PREFIX}/lib/lua/${LUA_VERSION}")
set(LUAMODULE_INSTALL_LIB_DIR "${CMAKE_INSTALL_PREFIX}/share/lua/${LUA_VERSION}")

set(etf_sources)
list(APPEND etf_sources "csrc/etf.c")

add_library(etf ${etf_sources})

if(WIN32)
    target_link_libraries(etf PRIVATE ${LUA_LIBRARIES})
endif()
target_include_directories(etf PRIVATE ${LUA_INCLUDE_DIR})

if(APPLE)
    set(CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS "${CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS} -undefined dynamic_lookup")
    if(BUILD_SHARED_LIBS)
        set_target_properties(etf PROPERTIES SUFFIX ".so")
    endif()
endif()

set_target_properties(etf PROPERTIES PREFIX "")
set_target_properties(etf PROPERTIES OUTPUT_NAME "etf")
set_target_properties(etf PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set_target_properties(etf PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set_target_properties(etf PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set_target_properties(etf PROPERTIES C_VISIBILITY_PRESET hidden)

foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
  string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
  set_target_properties(etf PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_BINARY_DIR}")
  set_target_properties(etf PROPERTIES LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_BINARY_DIR}")
  set_target_properties(etf PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_BINARY_DIR}")
endforeach()

install(TARGETS etf
  LIBRARY DESTINATION "${CMODULE_INSTALL_LIB_DIR}"
  RUNTIME DESTINATION "${CMODULE_INSTALL_LIB_DIR}"
  ARCHIVE DESTINATION "${CMODULE_INSTALL_LIB_DIR}"
)
