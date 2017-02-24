

include_directories(
    ${code_path}/plat_header
    ${code_path}/comm/include
    ${code_path}/sche/include
    ${code_path}/config/include
)

set(config_src ${code_path}/config/source)

aux_source_directory(${config_src} CONFIG_ALL_SRCS)

add_library(config STATIC ${CONFIG_ALL_SRCS})
set_target_properties(config PROPERTIES LINKER_LANGUAGE "C")
set_target_properties(config PROPERTIES OUTPUT_NAME "config")
