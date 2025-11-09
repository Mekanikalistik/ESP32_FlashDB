# FlashDB ESP-IDF Component

This document provides guidance on configuring and integrating the FlashDB library as a modular component within an ESP-IDF project.

## 1. Overview

The FlashDB component provides an ultra-lightweight embedded database solution for ESP-IDF applications. It leverages the Flash Abstraction Layer (FAL) to interact with ESP32's flash memory, offering Key-Value (KVDB) and Time Series Database (TSDB) functionalities.

## 2. Configuration

All primary FlashDB component configurations are managed through `components/FlashDB/inc/fdb_cfg.h`. This file contains a dedicated "FlashDB Configuration Section" where you can adjust various parameters.

### `components/FlashDB/inc/fdb_cfg.h`

This file is your central point for configuring FlashDB.

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
// For ESP32, the recommended value is 8 bits (1 byte) for robust operation.
// (e.g., 1 for NOR Flash, 8 for STM32F2/F4/ESP32)
#define FDB_WRITE_GRAN 8

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

/* ================================================================================================= */
/* ============================= End FlashDB Configuration Section ============================= */
/* ================================================================================================= */
```

### `components/FlashDB/inc/fal_cfg.h`

This file defines the logical partition table used by the Flash Abstraction Layer. It references the size macros defined in `fdb_cfg.h`.

```c
// Example content (actual content will vary based on fdb_cfg.h macros)
#define FAL_PART_TABLE                                                               \
{                                                                                    \
    {FAL_PART_MAGIC_WORD, "fdb_kvdb1", NOR_FLASH_DEV_NAME, 0,           FDB_KVDB1_SIZE, 0}, \
    {FAL_PART_MAGIC_WORD, "fdb_tsdb1", NOR_FLASH_DEV_NAME, FDB_KVDB1_SIZE,  FDB_TSDB1_SIZE, 0}, \
}
```
You should ensure that `NOR_FLASH_DEV_NAME` (which is `"norflash0"` by default in this file) is consistent with the Flash device name used in `fal_flash_esp32_port.c`.

## 3. Synchronization with `partitions.csv`

The FlashDB component expects a **physical flash partition named `flashdb`** to be defined in your project's `partitions.csv` file. This name is hardcoded in `components/FlashDB/port/fal_flash_esp32_port.c` and must match exactly.

### `partitions.csv` (Project Root)

Ensure your `partitions.csv` contains an entry similar to this:

```csv
# Name,   Type, SubType, Offset,  Size, Flags
flashdb,  0x40, 0x00,          ,  1M,
```

*   **`Name`**: Must be `flashdb`.
*   **`Type`**: Must be `0x40` (a custom data type for FlashDB).
*   **`SubType`**: Must be `0x00`.
*   **`Size`**: The total size of this partition must be **equal to or greater than** the sum of `FDB_KVDB1_SIZE` and `FDB_TSDB1_SIZE` defined in `fdb_cfg.h`. For example, if `FDB_KVDB1_SIZE` is 512KB and `FDB_TSDB1_SIZE` is 512KB, then the `flashdb` partition should be at least 1MB.

## 4. Usage in `main/CMakeLists.txt`

To use the FlashDB component in your main application, ensure your `main/CMakeLists.txt` declares a dependency on `FlashDB`:

```cmake
idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES FlashDB)
```

## 5. Using the API in your application

Include `flashdb.h` in your application files:

```c
#include <flashdb.h>
```

Then initialize and use the KVDB and TSDB APIs as demonstrated in the `samples/` directory.

## 6. Important Notes

*   **No Source Code Modification:** This component is designed to work without modifying any of the core FlashDB library source files.
*   **Warnings in Samples:** The provided sample files might generate compiler warnings (`-Wmaybe-uninitialized`) due to strict ESP-IDF compiler settings. These are handled by adding `-Wno-error=maybe-uninitialized` to your project's root `CMakeLists.txt`.
*   **Timestamp Format:** For 32-bit timestamps, ensure that any `printf` format specifiers used for `fdb_time_t` are `ld` (for `long int`).

## 7. How to Use in `main.c` (Example Integration)

This section provides a simplified guide on how to integrate and use the FlashDB component in your ESP-IDF `main.c` file. For a complete working example, refer to the project's `main/main.c` and `samples/` directories.

### Basic Initialization and Usage

```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_flash_spi_init.h" // Required for esp_flash_get_size
#include "spi_flash_mmap.h"     // Required for esp_flash_get_size
#include "esp_chip_info.h"      // Required for esp_chip_info

#include <flashdb.h> // Main FlashDB header

// --- FlashDB Global Variables ---
static uint32_t boot_count = 0;
// Example default KV nodes. These will be added if not already present.
static struct fdb_default_kv_node default_kv_table[] = {
   {"username", "armink", 0},                       /* string KV */
   {"password", "123456", 0},                       /* string KV */
   {"boot_count", &boot_count, sizeof(boot_count)}, /* int type KV */
};
// KVDB object instance
static struct fdb_kvdb kvdb = {0};
// TSDB object instance
struct fdb_tsdb tsdb = {0};
// Counter for simulated timestamp (replace with actual RTC in production)
static int s_tsdb_counts = 0;
// Semaphore for thread-safe access to FlashDB
static SemaphoreHandle_t s_flashdb_lock = NULL;

// --- FlashDB Lock/Unlock Functions (for thread safety) ---
static void lock_flashdb_access(fdb_db_t db)
{
   if (s_flashdb_lock) {
       xSemaphoreTake(s_flashdb_lock, portMAX_DELAY);
   }
}

static void unlock_flashdb_access(fdb_db_t db)
{
   if (s_flashdb_lock) {
       xSemaphoreGive(s_flashdb_lock);
   }
}

// --- Simulated Timestamp Function (replace with actual RTC in production) ---
static fdb_time_t get_current_timestamp(void)
{
   return ++s_tsdb_counts;
}

// --- FlashDB Demo Function ---
void flashdb_app_init(void)
{
   fdb_err_t result;

   // Initialize the FlashDB access semaphore
   if (s_flashdb_lock == NULL)
   {
       s_flashdb_lock = xSemaphoreCreateCounting(1, 1);
       assert(s_flashdb_lock != NULL);
   }

#ifdef FDB_USING_KVDB
   {
       struct fdb_default_kv default_kv;

       default_kv.kvs = default_kv_table;
       default_kv.num = sizeof(default_kv_table) / sizeof(default_kv_table[0]);

       // Set the lock and unlock functions for thread-safe access
       fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_LOCK, lock_flashdb_access);
       fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_UNLOCK, unlock_flashdb_access);

       // Initialize Key-Value database
       // "env": database name
       // "fdb_kvdb1": Logical partition name (from fal_cfg.h). Must be in FAL partition table.
       result = fdb_kvdb_init(&kvdb, "env", "fdb_kvdb1", &default_kv, NULL);

       if (result != FDB_NO_ERR) {
           ESP_LOGE("FlashDB", "KVDB initialization failed: %d", result);
       } else {
           ESP_LOGI("FlashDB", "KVDB initialized successfully.");
           // Example KV usage:
           // fdb_kv_set(&kvdb, "my_setting", "value");
           // char *value = fdb_kv_get(&kvdb, "my_setting");
       }
   }
#endif // FDB_USING_KVDB

#ifdef FDB_USING_TSDB
   {
       // Set the lock and unlock functions for thread-safe access
       fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_SET_LOCK, lock_flashdb_access);
       fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_SET_UNLOCK, unlock_flashdb_access);

       // Initialize Time Series Database
       // "log": database name
       // "fdb_tsdb1": Logical partition name (from fal_cfg.h). Must be in FAL partition table.
       // get_current_timestamp: Function to get current timestamp
       // 128: Maximum length of each log entry
       result = fdb_tsdb_init(&tsdb, "log", "fdb_tsdb1", get_current_timestamp, 128, NULL);

       if (result != FDB_NO_ERR) {
           ESP_LOGE("FlashDB", "TSDB initialization failed: %d", result);
       } else {
           ESP_LOGI("FlashDB", "TSDB initialized successfully.");
           // Example TSDB usage:
           // struct sensor_data { int temp; int humi; };
           // struct sensor_data data = {25, 60};
           // struct fdb_blob blob;
           // fdb_tsl_append(&tsdb, fdb_blob_make(&blob, &data, sizeof(data)));
       }
   }
#endif // FDB_USING_TSDB
}

// --- ESP-IDF Application Entry Point ---
void app_main()
{
   ESP_LOGI("APP", "Starting FlashDB ESP-IDF Demo");

   // Initialize FlashDB (KVDB and TSDB)
   flashdb_app_init();

   // You can call sample functions or integrate your own FlashDB logic here
   // For example:
   // kvdb_basic_sample(&kvdb);
   // tsdb_sample(&tsdb);

   // Main application loop (example: sleep and restart)
   for (int i = 1000; i >= 0; i--)
   {
       ESP_LOGI("APP", "Restarting in %d seconds...", i);
       vTaskDelay(1000 / portTICK_PERIOD_MS);
   }
   ESP_LOGI("APP", "Restarting now.");
   esp_restart();
}
```