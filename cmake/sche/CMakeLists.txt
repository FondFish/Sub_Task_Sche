
include_directories(
    ${code_path}/plat_header
    ${code_path}/comm/include
    ${code_path}/sche/include
    ${code_path}/config/include
)

set(sche_src ${code_path}/sche/source)

aux_source_directory(${sche_src} SCHE_ALL_SRCS)

add_library(sche STATIC ${SCHE_ALL_SRCS})
set_target_properties(sche PROPERTIES LINKER_LANGUAGE "C")
set_target_properties(sche PROPERTIES OUTPUT_NAME "sche")
