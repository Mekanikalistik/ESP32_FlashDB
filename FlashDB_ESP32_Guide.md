# Comprehensive Guide: Using FlashDB with ESP32-S3 & PlatformIO (ESP-IDF)

This guide provides a comprehensive walkthrough for setting up and using FlashDB on an ESP32-S3 microcontroller, specifically targeting the `heltec_wifi_lora_32_V3` board within the PlatformIO ecosystem using the ESP-IDF framework.

## 1. Introduction to FlashDB

FlashDB is a lightweight, embedded database designed for microcontrollers that provides both **Key-Value (KVDB)** and **Time-Series (TSDB)** database functionalities. It is built on top of the FAL (Flash Abstraction Layer), which provides a unified interface for various flash memory types.

### Key Concepts

*   **Key-Value Database (KVDB):** A simple, non-relational database that stores data as key-value pairs. Ideal for configuration, parameters, and state storage.
*   **Time-Series Database (TSDB):** Stores data points in chronological order, indexed by a timestamp. Perfect for logging sensor data, events, or any time-stamped information.
*   **FAL (Flash Abstraction Layer):** A crucial layer that abstracts the underlying flash hardware. FlashDB interacts with flash memory through FAL partitions, making it portable across different platforms.
*   **Blob:** A binary large object. In FlashDB, values are stored as blobs, allowing you to store any data type, from simple integers to complex structs.

## 2. Project Setup in PlatformIO

This guide assumes you have a PlatformIO project configured for the `heltec_wifi_lora_32_V3` board and the ESP-IDF framework. Standard ESP-IDF projects use a `main` directory for application code, which we will follow.

### 2.1. Integrating FlashDB as a Project Component

For a clean and automated setup, we will integrate FlashDB as a local component within the project. This process involves cloning the library, copying only the necessary source files, and then cleaning up.

**Step-by-step Integration:**

1.  **Clone the library into a temporary folder:**
    ```sh
    git clone https://github.com/armink/FlashDB.git temp_flashdb
    ```

2.  **Create the component directory structure:**
    ```sh
    mkdir -p components/FlashDB
    ```

3.  **Copy the essential source, include, and porting folders:**
    ```sh
    mv temp_flashdb/src components/FlashDB/
    mv temp_flashdb/inc components/FlashDB/
    mv temp_flashdb/port components/FlashDB/
    ```

4.  **Clean up the temporary directory:**
    ```sh
    rm -rf temp_flashdb
    ```

5.  **Create a `CMakeLists.txt` for the component:** Create a new file `components/FlashDB/CMakeLists.txt`. This is **essential** for the PlatformIO build system to compile the library.

    ```cmake
    # components/FlashDB/CMakeLists.txt
    idf_component_register(
        SRCS 
            "src/fdb_kvdb.c"
            "src/fdb_tsdb.c"
            "src/fdb_utils.c"
            "src/fdb.c"
            "port/fal/src/fal.c"
            "port/fal/src/fal_flash.c"
            "port/fal/src/fal_partition.c"
        INCLUDE_DIRS 
            "inc"
            "port/fal/inc"
    )
    ```

After these steps, your project structure should look like this:

```
- your_project/
  - components/
    - FlashDB/
      - src/
      - inc/
      - port/
      - CMakeLists.txt  # <-- Important
  - main/
    - main.c
  - platformio.ini
  - ...
```

### 2.2. Configuring the Flash Partition Table

FlashDB requires a dedicated partition on the flash memory. For the ESP32-S3 with 8MB of flash, you need to define a custom partition table.

1.  **Create `partitions.csv`:** In the root of your project, create a file named `partitions.csv`. This example allocates 256KB for a KVDB and 256KB for a TSDB.

    ```csv
    # Name,   Type, SubType, Offset,  Size, Flags
    nvs,      data, nvs,     ,        24K,
    otadata,  data, ota,     ,        8K,
    app0,     app,  ota_0,   ,        3M,
    fdb_kvdb, data, spiffs,  ,        256K,
    fdb_tsdb, data, spiffs,  ,        256K,
    spiffs,   data, spiffs,  ,        remaining,
    ```

2.  **Update `platformio.ini`:** Tell PlatformIO to use this custom partition table by adding the `board_build.partitions` line to your `platformio.ini` file:

    ```ini
    [env:heltec_wifi_lora_32_V3]
    platform = espressif32
    board = heltec_wifi_lora_32_V3
    framework = espidf
    board_build.partitions = partitions.csv
    ```

## 3. Porting and Configuration

This is the most critical part of the setup. You will need to create and modify a few files to make FlashDB work with your specific board.

#### Summary of Files to Create/Modify:

*   **`platformio.ini` (Modify):** To add the custom partition table.
*   **`partitions.csv` (Create):** To define the flash layout for your 8MB ESP32-S3.
*   **`components/FlashDB/CMakeLists.txt` (Create):** To make the library buildable.
*   **`main/fal_cfg.h` (Create):** To configure the Flash Abstraction Layer (FAL) partitions.
*   **`main/fdb_cfg.h` (Create):** To configure FlashDB features (KVDB/TSDB). You can start by copying `components/FlashDB/inc/fdb_cfg_template.h`.
*   **`main/fal_flash_esp32_port.c` (Create):** To provide the low-level flash driver for FAL.
*   **`main/CMakeLists.txt` (Modify):** To include the new porting files in the build.

### 3.1. FAL (Flash Abstraction Layer) Porting

The core of the porting effort is to implement the FAL interfaces for the ESP32's SPI flash. The following configuration files should be placed in your `main` directory alongside your application code.

1.  **Create `fal_cfg.h`:** Inside your `main` directory, create `fal_cfg.h`. This file tells FAL which flash partitions FlashDB can use.

    ```c
    // main/fal_cfg.h
    #ifndef _FAL_CFG_H_
    #define _FAL_CFG_H_

    #include <esp_spi_flash.h>

    /* Flash device Configuration */
    extern const struct fal_flash_dev esp32_onchip_flash;

    /* flash device table */
    #define FAL_FLASH_DEV_TABLE      \
    {                                \
        &esp32_onchip_flash,         \
    }

    /* Partition Configuration */
    #ifdef FAL_PART_HAS_TABLE_CFG
    #define FAL_PART_TABLE                                                                \
    {                                                                                     \
        {FAL_PART_MAGIC_WORD, "fdb_kvdb", "esp32_onchip", 0, 256 * 1024, 0}, \
        {FAL_PART_MAGIC_WORD, "fdb_tsdb", "esp32_onchip", 256 * 1024, 256 * 1024, 0}, \
    }
    #endif /* FAL_PART_HAS_TABLE_CFG */

    #endif /* _FAL_CFG_H_ */
    ```
    **Important:** The partition names (`fdb_kvdb`, `fdb_tsdb`) and sizes in `fal_cfg.h` must correspond to the `partitions.csv` file.

2.  **Implement FAL Port (`fal_flash_esp32_port.c`):** Create a new file named `fal_flash_esp32_port.c` in your `main` directory. This file provides the actual driver that connects FAL to the ESP-IDF's flash functions. The code below is specifically for ESP32/ESP32-S3 and should be used as-is.

    ```c
    // main/fal_flash_esp32_port.c
    #include <fal.h>
    #include "esp_spi_flash.h"

    static int init(void)
    {
        // spi_flash_init() is already called by the ESP-IDF bootloader
        return 1;
    }

    static int read(long offset, uint8_t *buf, size_t size)
    {
        if (spi_flash_read(offset, buf, size) == ESP_OK) {
            return size;
        }
        return -1;
    }

    static int write(long offset, const uint8_t *buf, size_t size)
    {
        if (spi_flash_write(offset, buf, size) == ESP_OK) {
            return size;
        }
        return -1;
    }

    static int erase(long offset, size_t size)
    {
        if (spi_flash_erase_range(offset, size) == ESP_OK) {
            return size;
        }
        return -1;
    }

    const struct fal_flash_dev esp32_onchip_flash =
    {
        .name       = "esp32_onchip",
        .addr       = 0,
        .len        = 8 * 1024 * 1024, // 8MB for ESP32-S3
        .blk_size   = 4096,
        .ops        = {init, read, write, erase},
        .write_gran = 1
    };
    ```

### 3.2. FlashDB Configuration (`fdb_cfg.h`)

Create `fdb_cfg.h` in your `main` directory. The easiest way to create it is to **copy `components/FlashDB/inc/fdb_cfg_template.h`** to your `main` directory and rename it to `fdb_cfg.h`.

Then, ensure the following macros are defined:

```c
// main/fdb_cfg.h
#ifndef _FDB_CFG_H_
#define _FDB_CFG_H_

/* Enable KVDB feature */
#define FDB_USING_KVDB

/* Enable TSDB feature */
#define FDB_USING_TSDB

/* Enable FAL mode */
#define FDB_USING_FAL_MODE

/* Flash write granularity, unit: bit. For ESP32/S3 SPI Flash, this is 1 */
#define FDB_WRITE_GRAN         1

/* Enable debug information output */
#define FDB_DEBUG_ENABLE

#endif /* _FDB_CFG_H_ */
```

## 4. Using FlashDB in Your Application

### 4.1. Build System and Includes

For the build system to find your new porting files, you must add them to your main component's `CMakeLists.txt`.

Modify your `main/CMakeLists.txt` file (PlatformIO generates this automatically):

```cmake
# main/CMakeLists.txt
idf_component_get_property(main_dir ${COMPONENT_NAME} COMPONENT_DIR)

idf_component_register(SRCS "main.c"
                       "fal_flash_esp32_port.c" #<-- Add the porting file
                  INCLUDE_DIRS "."
                               ${main_dir}     #<-- Add main to includes
)
```

### 4.2. Initialization

In your `main/main.c`, you must first initialize FAL, then initialize the databases you want to use.

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

### 4.3. KVDB Usage Examples

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

### 4.4. TSDB Usage Examples

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

## 5. Conclusion

This guide provides a complete roadmap for integrating FlashDB into your ESP32-S3 project using PlatformIO and ESP-IDF. By following these steps, you can leverage a powerful and lightweight database solution for your embedded applications, enabling persistent storage for configuration, state, and time-series data. Always refer to the official FlashDB documentation for more advanced API usage and features.

## 6. References

This guide was compiled by referencing the following documents from the FlashDB repository:

*   [`docs/quick-started.md`](docs/quick-started.md)
*   [`docs/demo-details.md`](docs/demo-details.md)
*   [`docs/porting.md`](docs/porting.md)
*   [`docs/configuration.md`](docs/configuration.md)
*   [`docs/api.md`](docs/api.md)
*   [`port/fal/docs/fal_api_en.md`](port/fal/docs/fal_api_en.md)
*   [`demos/esp32_spi_flash/README.md`](demos/esp32_spi_flash/README.md)
*   [`docs/sample-kvdb-basic.md`](docs/sample-kvdb-basic.md)
*   [`docs/sample-kvdb-type-string.md`](docs/sample-kvdb-type-string.md)
*   [`docs/sample-kvdb-type-blob.md`](docs/sample-kvdb-type-blob.md)
*   [`docs/sample-kvdb-traversal.md`](docs/sample-kvdb-traversal.md)
*   [`docs/sample-tsdb-basic.md`](docs/sample-tsdb-basic.md)