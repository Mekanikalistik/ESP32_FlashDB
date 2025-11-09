set(SOURCES
    "src/fdb.c"
    "src/fdb_kvdb.c"
    "src/fdb_tsdb.c"
    "src/fdb_utils.c"
    "port/fal/src/fal.c"
    "port/fal/src/fal_flash.c"
    "port/fal/src/fal_partition.c"
    "port/fal_flash_esp32_port.c"
)

set(INCLUDE_DIRS
    "inc"
    "port/fal/inc"
)

idf_component_register(SRCS "${SOURCES}"
                    INCLUDE_DIRS "${INCLUDE_DIRS}"
                    REQUIRES spi_flash esp_partition esp_rom)

target_include_directories(${COMPONENT_LIB} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/inc")