

include_directories(
    ${code_path}/plat_header
    ${code_path}/comm/include
    ${code_path}/sche/include
    ${code_path}/config/include
)

set(comm_src ${code_path}/comm/source)

aux_source_directory(${comm_src} COMMON_ALL_SRCS)

add_library(comm STATIC ${COMMON_ALL_SRCS})
set_target_properties(comm PROPERTIES LINKER_LANGUAGE "C")
set_target_properties(comm PROPERTIES OUTPUT_NAME "comm")
