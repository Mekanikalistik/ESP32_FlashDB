/*
 * Copyright (c) 2022, kaans, <https://github.com/kaans>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file
 * @brief configuration file
 */
#ifndef _FDB_CFG_H_
#define _FDB_CFG_H_

/* ================================================================================================= */
/* =============================== FlashDB Configuration Section =============================== */
/* ================================================================================================= */

/* --- Core Feature Configuration --- */
/* using KVDB (Key-Value Database) feature */
#define FDB_USING_KVDB

#ifdef FDB_USING_KVDB
/* Auto update KV to latest default when current KVDB version number is changed. @see fdb_kvdb.ver_num */
/* #define FDB_KV_AUTO_UPDATE */
#endif

/* using TSDB (Time Series Database) feature */
#define FDB_USING_TSDB

/* --- Storage Mode Configuration --- */
/* Using FAL (Flash Abstraction Layer) storage mode */
#define FDB_USING_FAL_MODE

/* --- Flash Parameters Configuration --- */
/* The flash write granularity, unit: bit.
 * This value depends on your flash chip's smallest programmable unit.
 * For ESP32 SPI flash, the recommended value is 1 bit for robust operation. */
#define FDB_WRITE_GRAN 1

/* --- Timestamp Configuration --- */
// Uncomment to enable 64-bit timestamps for TSDB. Default is 32-bit.
// If enabled, fdb_time_t will be int64_t. Otherwise, it will be int32_t.
// #define FDB_USING_TIMESTAMP_64BIT

/* --- Logical Partition Sizes (used in fal_cfg.h) --- */
// Define the desired sizes for your KVDB and TSDB logical partitions.
// Ensure their sum does not exceed the size of the physical "flashdb" partition in partitions.csv.
#define FDB_KVDB1_SIZE (512 * 1024) // Size for KVDB partition (bytes)
#define FDB_TSDB1_SIZE (512 * 1024) // Size for TSDB partition (bytes)

// Total size of the FlashDB partition in partitions.csv.
// This must be equal to or greater than the sum of FDB_KVDB1_SIZE and FDB_TSDB1_SIZE.
#define FDB_TOTAL_PARTITION_SIZE (FDB_KVDB1_SIZE + FDB_TSDB1_SIZE)


/* ================================================================================================= */
/* ============================= End FlashDB Configuration Section ============================= */
/* ================================================================================================= */


/* --- Internal/Toolchain Configuration (usually no need to change) --- */

/* MCU Endian Configuration, default is Little Endian Order. */
/* #define FDB_BIG_ENDIAN  */

/* log print macro using ESP_LOG.
 * Need to include esp_log.h when setting FDB_PRINT to ESP_LOGI().
 * default EF_PRINT macro is printf() */
#include <esp_log.h>
#define FDB_PRINT(...) ESP_LOGI("fdb", __VA_ARGS__)

/* print debug information */
#define FDB_DEBUG_ENABLE

#endif /* _FDB_CFG_H_ */
