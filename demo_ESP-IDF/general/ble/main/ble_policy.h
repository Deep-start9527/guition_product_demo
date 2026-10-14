#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define BLE_DATA_MAX 512
#define BLE_PREVIEW_MAX 64
#define BLE_HISTORY_MAX 32
#define BLE_QUEUE_DEPTH 8
typedef struct {
    bool connected, subscribed, encrypted, authenticated, bonded;
    uint8_t key_size;
    uint16_t mtu;
    uint32_t generation;
} ble_link_t;
enum { BLE_POLICY_OK, BLE_POLICY_LENGTH, BLE_POLICY_SECURITY, BLE_POLICY_LINK,
       BLE_POLICY_SUBSCRIPTION, BLE_POLICY_STALE, BLE_POLICY_TIMEOUT, BLE_POLICY_SEND };
bool ble_policy_secure(const ble_link_t *link, bool secure_mode);
int ble_policy_rx(const ble_link_t *link, bool secure_mode, size_t len);
int ble_policy_tx(const ble_link_t *link, bool secure_mode, uint32_t generation);
size_t ble_policy_chunk(uint16_t mtu, size_t remaining);
typedef struct {
    void *ctx;
    uint64_t (*now_ms)(void *);
    void (*pause_ms)(void *, unsigned);
    // Returns 0 on accepted, 1 on temporary resource shortage, other on failure.
    int (*submit)(void *, const uint8_t *, size_t, uint32_t);
    int (*check)(void *, uint32_t, uint16_t *);
} ble_send_hooks_t;
// One complete NUS byte stream send. accepted reports only stack submissions.
int ble_policy_send(const ble_send_hooks_t *hooks, const uint8_t *data, size_t len,
                    uint32_t generation, size_t *accepted);
