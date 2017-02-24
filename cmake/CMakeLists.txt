cmake_minimum_required(VERSION 3.2.2)

project(sche)

set(cur_dir ${CMAKE_CURRENT_LIST_DIR})
message("cur_dir = ${cur_dir}")
STRING(LENGTH ${cur_dir} dir_len)
MATH(EXPR len ${dir_len}-6)
STRING(SUBSTRING ${cur_dir} 0, ${len} project_path)
message("project_path = ${project_path}")

set(code_path ${project_path}/code)

set(main_src ${code_path}/main.c)

include_directories(
    ${code_path}/plat_header
    ${code_path}/comm/include
    ${code_path}/sche/include
    ${code_path}/config/include
)

add_subdirectory(comm)
add_subdirectory(config)
add_subdirectory(sche)

set(EXE_DEPEND_LIBS comm config sche comm pthread rt m stdc++)
list(APPEND ALL_LINK_LIBS_DIR
    ${cur_dir}/comm
    ${cur_dir}/config
    ${cur_dir}/sche
)
message("ALL_LINK_LIBS_DIR = ${ALL_LINK_LIBS_DIR}")
link_directories(${ALL_LINK_LIBS_DIR})

add_executable(main_exe ${main_src})
target_link_libraries(main_exe ${EXE_DEPEND_LIBS} )
set_target_properties(main_exe PROPERTIES LINKER_LANGUAGE "C")
