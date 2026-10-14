#include "ble_policy.h"
bool ble_policy_secure(const ble_link_t *l, bool secure)
{
    return !secure || (l->encrypted && l->authenticated && l->bonded && l->key_size >= 16);
}
int ble_policy_rx(const ble_link_t *l, bool secure, size_t len)
{
    if (!l->connected) return BLE_POLICY_LINK;
    if (!ble_policy_secure(l, secure)) return BLE_POLICY_SECURITY;
    if (l->mtu < 23 || len > BLE_DATA_MAX || len > (size_t)(l->mtu - 3)) return BLE_POLICY_LENGTH;
    return BLE_POLICY_OK;
}
int ble_policy_tx(const ble_link_t *l, bool secure, uint32_t gen)
{
    if (gen != l->generation) return BLE_POLICY_STALE;
    if (!l->connected) return BLE_POLICY_LINK;
    if (!ble_policy_secure(l, secure)) return BLE_POLICY_SECURITY;
    return l->subscribed ? BLE_POLICY_OK : BLE_POLICY_SUBSCRIPTION;
}
size_t ble_policy_chunk(uint16_t mtu, size_t remaining)
{
    if (mtu < 23) return 0;
    size_t n = mtu - 3;
    return remaining < n ? remaining : n;
}
int ble_policy_send(const ble_send_hooks_t *h, const uint8_t *data, size_t len,
                    uint32_t gen, size_t *accepted)
{
    *accepted = 0;
    if (len > BLE_DATA_MAX) return BLE_POLICY_LENGTH;
    uint64_t deadline = h->now_ms(h->ctx) + 2000;
    bool first = true;
    while (first || *accepted < len) {
        first = false;
        uint16_t mtu = 0;
        int rc = h->check(h->ctx, gen, &mtu);
        if (rc) return rc;
        if (h->now_ms(h->ctx) >= deadline) return BLE_POLICY_TIMEOUT;
        size_t chunk = ble_policy_chunk(mtu, len - *accepted);
        if (!chunk && len) return BLE_POLICY_LENGTH;
        rc = h->submit(h->ctx, data + *accepted, chunk, gen);
        if (rc == 1) {
            first = true;
            h->pause_ms(h->ctx, 20);
            continue;
        }
        if (rc) return BLE_POLICY_SEND;
        *accepted += chunk;
    }
    return BLE_POLICY_OK;
}
