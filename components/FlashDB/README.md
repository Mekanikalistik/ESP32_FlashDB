# FlashDB ESP-IDF Component

This guide provides a comprehensive walkthrough for setting up and using FlashDB on an ESP32-S3 microcontroller within the ESP-IDF framework. It covers essential configuration, partition management, and usage examples.

## 1. Introduction to FlashDB

FlashDB is a lightweight, embedded database designed for microcontrollers that provides both **Key-Value (KVDB)** and **Time-Series (TSDB)** database functionalities. It is built on top of the FAL (Flash Abstraction Layer), which provides a unified interface for various flash memory types.

### Key Concepts

*   **Key-Value Database (KVDB):** A simple, non-relational database that stores data as key-value pairs. Ideal for configuration, parameters, and state storage.
*   **Time-Series Database (TSDB):** Stores data points in chronological order, indexed by a timestamp. Perfect for logging sensor data, events, or any time-stamped information.
*   **FAL (Flash Abstraction Layer):** A crucial layer that abstracts the underlying flash hardware. FlashDB interacts with flash memory through FAL partitions, making it portable across different platforms.
*   **Blob:** A binary large object. In FlashDB, values are stored as blobs, allowing you to store any data type, from simple integers to complex structs.

## 2. Configuration

All primary FlashDB component configurations are managed through `components/FlashDB/inc/fdb_cfg.h` and `components/FlashDB/inc/fal_cfg.h`. These files contain dedicated sections where you can adjust various parameters to suit your project's needs.

### `components/FlashDB/inc/fdb_cfg.h`

This file is your central point for configuring FlashDB's core features and parameters.

```c
/* ================================================================================================= */
/* =============================== FlashDB Configuration Section =============================== */
/* ================================================================================================= */

/* --- Core Feature Configuration --- */
// Enable or disable Key-Value Database (KVDB) feature
#define FDB_USING_KVDB

// Enable or disable Time Series Database (TSDB) feature
#define FDB_USING_TSDB

/* --- Storage Mode Configuration --- */
// Enable FAL (Flash Abstraction Layer) storage mode for flash-based operations
#define FDB_USING_FAL_MODE

/* --- Flash Parameters Configuration --- */
// The flash write granularity, unit: bit.
// This value depends on your flash chip's smallest programmable unit.
// For ESP32 SPI flash, the recommended value is 1 bit for robust operation.
#define FDB_WRITE_GRAN 1

/* --- Timestamp Configuration --- */
// Uncomment to enable 64-bit timestamps (int64_t) for TSDB.
// If commented, 32-bit timestamps (int32_t) will be used.
// 64-bit timestamps prevent the Year 2038 problem and allow for higher precision over longer periods.
// #define FDB_USING_TIMESTAMP_64BIT

/* --- Logical Partition Sizes (used in fal_cfg.h) --- */
// Define the desired sizes for your KVDB and TSDB logical partitions.
// The sum of these sizes must NOT exceed the total size of the physical "flashdb" partition
// defined in your project's `partitions.csv`.
#define FDB_KVDB1_SIZE (512 * 1024) // Size for KVDB partition (bytes)
#define FDB_TSDB1_SIZE (512 * 1024) // Size for TSDB partition (bytes)

// Define the total size of the "flashdb" partition as the sum of logical partitions.
// This macro is used in `fal_flash_esp32_port.c` to define the flash device length.
#define FDB_TOTAL_PARTITION_SIZE (FDB_KVDB1_SIZE + FDB_TSDB1_SIZE)

/* ================================================================================================= */
/* ============================= End FlashDB Configuration Section ============================= */
/* ================================================================================================= */

/* --- Internal/Toolchain Configuration (usually no need to change) --- */

/* log print macro using ESP_LOG.
 * Need to include esp_log.h in fdb_def.h when setting FDB_PRINT to ESP_LOGI().
 * default EF_PRINT macro is printf() */
#define FDB_PRINT(...) ESP_LOGI("fdb", __VA_ARGS__)

/* print debug information */
#define FDB_DEBUG_ENABLE
```

### `components/FlashDB/inc/fal_cfg.h`

This file configures the Flash Abstraction Layer (FAL), defining flash devices and logical partitions.

```c
// Example content (actual content will vary based on fdb_cfg.h macros)
#define FAL_PART_HAS_TABLE_CFG
#define NOR_FLASH_DEV_NAME "esp32_flash" // Consistent flash device name

#define FAL_PART_TABLE                                                               \
{                                                                                    \
    {FAL_PART_MAGIC_WORD, "fdb_kvdb1", NOR_FLASH_DEV_NAME, 0,           FDB_KVDB1_SIZE, 0}, \
    {FAL_PART_MAGIC_WORD, "fdb_tsdb1", NOR_FLASH_DEV_NAME, FDB_KVDB1_SIZE,  FDB_TSDB1_SIZE, 0}, \
}
```
Ensure that `NOR_FLASH_DEV_NAME` is consistent with the Flash device name used in `fal_flash_esp32_port.c`.

### `components/FlashDB/port/fal_flash_esp32_port.c`

This file provides the ESP32-specific porting layer for FAL, interfacing with the ESP-IDF `esp_partition` API.

It defines the physical flash device (`esp32_flash`) and its operations (`init`, `read`, `write`, `erase`).

## 3. Partition Table (`partitions.csv`)

The FlashDB component expects a **physical flash partition named `flashdb`** to be defined in your project's `partitions.csv` file. This name is used by the `fal_flash_esp32_port.c` to locate the physical flash region.

### `partitions.csv` (Project Root)

Ensure your `partitions.csv` contains an entry similar to this:

```csv
# Name,   Type, SubType, Offset,  Size, Flags
flashdb,  0x40, 0x00,          ,  1M,    // Custom FlashDB partition
```

*   **`Name`**: Must be `flashdb`.
*   **`Type`**: Must be `0x40` (a custom data type for FlashDB).
*   **`SubType`**: Must be `0x00`.
*   **`Size`**: The total size of this partition must be **equal to or greater than** the sum of `FDB_KVDB1_SIZE` and `FDB_TSDB1_SIZE` defined in `fdb_cfg.h`. For example, if `FDB_KVDB1_SIZE` is 512KB and `FDB_TSDB1_SIZE` is 512KB, then the `flashdb` partition should be at least 1MB.

## 4. CMake Configuration

To correctly build the FlashDB component and your main application, ensure your `CMakeLists.txt` files declare the necessary dependencies.

### `components/FlashDB/CMakeLists.txt`

```cmake
idf_component_register(SRCS "${SOURCES}"
                    INCLUDE_DIRS "${INCLUDE_DIRS}"
                    REQUIRES spi_flash esp_partition esp_rom log freertos esp_system)
```
The FlashDB component explicitly requires `spi_flash`, `esp_partition`, `esp_rom`, `log`, `freertos`, and `esp_system` for its full functionality and to ensure it is self-contained.

### `main/CMakeLists.txt`

```cmake
idf_component_register(SRCS "main.c"
                           "../samples/kvdb_basic_sample.c"
                           "../samples/kvdb_type_blob_sample.c"
                           "../samples/kvdb_type_string_sample.c"
                           "../samples/tsdb_sample.c"
                    INCLUDE_DIRS "."
                    REQUIRES FlashDB nvs_flash spi_flash freertos esp_system log)
```
The main application's `CMakeLists.txt` declares dependencies that `main.c` directly uses, as well as the `FlashDB` component. Based on `main.c`'s includes, the required components are `FlashDB`, `nvs_flash`, `spi_flash`, `freertos`, `esp_system`, and `log`. `esp_partition` and `esp_rom` are handled by the FlashDB component's `CMakeLists.txt`.

## 5. Using FlashDB in Your Application

### 5.1. Initialization

In your `main.c`, you must first initialize FAL, then initialize the databases you want to use.

```c
#include <stdio.h>
#include <time.h>
#include <fal.h>
#include <flashdb.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

static struct fdb_kvdb kvdb;
static struct fdb_tsdb tsdb;
static SemaphoreHandle_t kvdb_lock;
static SemaphoreHandle_t tsdb_lock;

static void lock(fdb_db_t db) {
    if (db->type == FDB_DB_TYPE_KV) {
        xSemaphoreTake(kvdb_lock, portMAX_DELAY);
    } else {
        xSemaphoreTake(tsdb_lock, portMAX_DELAY);
    }
}

static void unlock(fdb_db_t db) {
    if (db->type == FDB_DB_TYPE_KV) {
        xSemaphoreGive(kvdb_lock);
    } else {
        xSemaphoreGive(tsdb_lock);
    }
}

static fdb_time_t get_time(void) {
    return time(NULL);
}


void app_main(void)
{
    fal_init();

    // -- KVDB Initialization --
    kvdb_lock = xSemaphoreCreateMutex();
    fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_LOCK, lock);
    fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_UNLOCK, unlock);

    fdb_err_t result = fdb_kvdb_init(&kvdb, "env", "fdb_kvdb", NULL, NULL);
    if (result != FDB_NO_ERR) {
        printf("Failed to initialize KVDB\n");
        return;
    }

    // -- TSDB Initialization --
    tsdb_lock = xSemaphoreCreateMutex();
    fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_SET_LOCK, lock);
    fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_SET_UNLOCK, unlock);

    result = fdb_tsdb_init(&tsdb, "log", "fdb_tsdb", get_time, 128, NULL);
    if (result != FDB_NO_ERR) {
        printf("Failed to initialize TSDB\n");
        return;
    }

    // Now you can use the databases
    // ... example usage ...
}
```

### 5.2. KVDB Usage Examples

#### Storing a Simple Value (Blob)

```c
// Store an integer
int boot_count = 0;
struct fdb_blob blob;

// Get the current value
fdb_kv_get_blob(&kvdb, "boot_count", fdb_blob_make(&blob, &boot_count, sizeof(boot_count)));

// Increment and save back
boot_count++;
fdb_kv_set_blob(&kvdb, "boot_count", fdb_blob_make(&blob, &boot_count, sizeof(boot_count)));
printf("Boot count: %d\n", boot_count);
```

#### Storing a String

```c
// Set a string value
fdb_kv_set(&kvdb, "device_id", "esp32-s3-lora-01");

// Get a string value
char *device_id = fdb_kv_get(&kvdb, "device_id");
if (device_id) {
    printf("Device ID: %s\n", device_id);
}
```

### 5.3. TSDB Usage Examples

#### Storing Sensor Data

```c
// Define a struct for your sensor data
struct sensor_reading {
    float temperature;
    float humidity;
};

// Append a new log entry
struct sensor_reading reading = { .temperature = 25.5, .humidity = 60.2 };
struct fdb_blob blob;
fdb_tsl_append(&tsdb, fdb_blob_make(&blob, &reading, sizeof(reading)));
```

#### Iterating and Querying Logs

```c
// Define a callback function to process each log entry
static bool ts_query_cb(fdb_tsl_t tsl, void *arg) {
    struct sensor_reading reading;
    struct fdb_blob blob;

    fdb_blob_read((fdb_db_t)&tsdb, fdb_tsl_to_blob(tsl, fdb_blob_make(&blob, &reading, sizeof(reading))));
    
    printf("TSL at time %d - Temp: %.2f, Humi: %.2f\n", (int)tsl->time, reading.temperature, reading.humidity);
    
    return false; // continue iteration
}

// Iterate over all logs in the TSDB
fdb_tsl_iter(&tsdb, ts_query_cb, NULL);
```

## 6. Important Notes

*   **No Core Source Code Modification:** This component is designed to work without modifying any of the core FlashDB library source files (`fal.c`, `fal_partition.c`, `fdb.c`, etc.). All ESP-IDF specific adaptations are confined to `fal_flash_esp32_port.c` and configuration headers.
*   **`FDB_WRITE_GRAN`:** For ESP32 SPI flash, `FDB_WRITE_GRAN` should be set to `1` bit in `fdb_cfg.h`. This value reflects the smallest programmable unit of the flash, ensuring optimal performance and data integrity with FlashDB's internal mechanisms.
*   **`FDB_TOTAL_PARTITION_SIZE`:** This macro, defined in `fdb_cfg.h`, represents the total size of the physical "flashdb" partition in `partitions.csv`. It is crucial for correctly configuring the flash device length in `fal_flash_esp32_port.c`.
*   **Explicit `fal_init()`:** Ensure `fal_init()` is called explicitly in your `main.c` before initializing FlashDB databases to properly initialize the Flash Abstraction Layer and detect partitions.
*   **Error Handling:** The `fal_flash_esp32_port.c` includes robust error handling for partition discovery, logging errors with `ESP_LOGE` and returning appropriate error codes.
*   **Logging (`ESP_LOGI`):** FlashDB's logging (`FDB_PRINT`) is configured to use ESP-IDF's `ESP_LOGI` for better integration with the system's logging framework. Ensure `esp_log.h` is included in any file using `FDB_PRINT` if not already included by other means (e.g., in `fal_flash_esp32_port.c`).
*   **Warnings in Samples:** The provided sample files might generate compiler warnings (`-Wmaybe-uninitialized`) due to strict ESP-IDF compiler settings. These are often handled by adding `-Wno-error=maybe-uninitialized` to your project's root `CMakeLists.txt`.
*   **Timestamp Format:** For 32-bit timestamps, ensure that any `printf` format specifiers used for `fdb_time_t` are `ld` (for `long int`).