idf_component_register(SRCS "main.c"
                           "../samples/kvdb_basic_sample.c"
                           "../samples/kvdb_type_blob_sample.c"
                           "../samples/kvdb_type_string_sample.c"
                           "../samples/tsdb_sample.c"
                    INCLUDE_DIRS "."
                    REQUIRES FlashDB nvs_flash spi_flash freertos esp_system log)