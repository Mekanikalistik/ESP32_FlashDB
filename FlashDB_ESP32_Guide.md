# Comprehensive Guide: Using FlashDB with ESP32-S3 & PlatformIO (ESP-IDF)

This guide provides a comprehensive walkthrough for setting up and using FlashDB on an ESP32-S3 microcontroller, specifically targeting the `heltec_wifi_lora_32_V3` board within the PlatformIO ecosystem using the ESP-IDF framework.

## 1. Introduction to FlashDB

FlashDB is a lightweight, embedded database designed for microcontrollers that provides both **Key-Value (KVDB)** and **Time-Series (TSDB)** database functionalities. It is built on top of the FAL (Flash Abstraction Layer), which provides a unified interface for various flash memory types.

### Key Concepts

*   **Key-Value Database (KVDB):** A simple, non-relational database that stores data as key-value pairs. Ideal for configuration, parameters, and state storage.
*   **Time-Series Database (TSDB):** Stores data points in chronological order, indexed by a timestamp. Perfect for logging sensor data, events, or any time-stamped information.
*   **FAL (Flash Abstraction Layer):** A crucial layer that abstracts the underlying flash hardware. FlashDB interacts with flash memory through FAL partitions, making it portable across different platforms.
*   **Blob:** A binary large object. In FlashDB, values are stored as blobs, allowing you to store any data type, from simple integers to complex structs.



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