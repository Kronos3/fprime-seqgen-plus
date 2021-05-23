cmake_minimum_required(VERSION 3.0)
project(cc VERSION 1.0)

enable_language(CXX)
set(CXX_STANDARD 11)

add_subdirectory(neoast)

BuildParser(cc_lib grammar/grammar.y CXX)
add_executable(cc
        cc.h
        grammar/grammar.cc
        grammar/grammar.h
        compilation/context.cc
        compilation/context.h
        compilation/traversal.cc
        common/common.h
        compilation/main.cc
        ${cc_lib_OUTPUT}
        compilation/compile.cc compilation/compile.h common/common.cc compilation/instruction.cc compilation/instruction.h compilation/module.h compilation/module.cc compilation/type.cc compilation/type.h)

set_target_properties(cc PROPERTIES LINKER_LANGUAGE CXX)

add_library(cc_dbg STATIC
        debug/print_ast.cc
        debug/print_debug.h
        debug/print_ir.cc)

target_link_libraries(cc neoast cc_dbg)
target_include_directories(cc_dbg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(cc PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR})
target_compile_options(cc PRIVATE -Werror)
