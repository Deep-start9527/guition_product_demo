/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 0)
#include "esp_random.h"
#else
#include "esp_system.h"
#endif

#include "wifi_cmd.h"

#if WIFI_CMD_OFFCHANNEL_SUPPORTED
#ifndef APP_TAG
#define APP_TAG "WIFI"
#endif
static int offchannel_rx_callback(uint8_t *hdr, uint8_t *payload, size_t len, uint8_t channel)
{
    ESP_LOGI(APP_TAG, "RECVLEN:%d,CHANNEL:%d", len, channel);
    return 0;
}

typedef struct {
    struct arg_str *action;
    struct arg_str *ifx;
    struct arg_str *mac;
    struct arg_int *type;
    struct arg_int *number;
    struct arg_int *channel;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 5, 0)
    struct arg_str *sec_channel;
#endif
    struct arg_int *wait_time_ms;
    struct arg_lit *no_ack;
    struct arg_int *op_id;
    struct arg_int *data_len;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 0, 0)
    struct arg_str *bssid;
    struct arg_lit *allow_broadcast;
#endif
    struct arg_end *end;
} wifi_offchannel_args_t;
static wifi_offchannel_args_t wifi_offchannel_args;

typedef enum {
    WIFI_ACTION_OFFCHANNEL_TX,
    WIFI_ACTION_REMAIN_ON_CHANNEL,
} wifi_offchannel_action_t;

static int cmd_do_wifi_offchannel(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **) &wifi_offchannel_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, wifi_offchannel_args.end, argv[0]);
        return 1;
    }

    esp_err_t ret = ESP_FAIL;
    uint8_t ifx = 0;
    uint8_t mac[6] = {0};
    uint8_t type = 1;
    uint8_t channel = 0;
    uint8_t count = 1;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 5, 2)  /* 44a66c7521832fd5f6b907c30b6e1ec3924f9baf */
    uint8_t sec_channel = 0;
#endif
    uint32_t wait_time_ms = 200;
    bool no_ack = false;
    uint32_t op_id = 0;
    uint16_t data_len = 0;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 0, 0)
    uint8_t bssid[6] = {0};
    bool allow_broadcast = false;
#endif
    wifi_offchannel_action_t action = WIFI_ACTION_OFFCHANNEL_TX;
    wifi_action_rx_cb_t rx_cb = offchannel_rx_callback;

    if (strcmp(wifi_offchannel_args.action->sval[0], "send") == 0) {
        action = WIFI_ACTION_OFFCHANNEL_TX;
    } else if (strcmp(wifi_offchannel_args.action->sval[0], "roc") == 0) {
        action = WIFI_ACTION_REMAIN_ON_CHANNEL;
    } else {
        ESP_LOGE(APP_TAG, "invalid action");
        return 1;
    }

    if (wifi_offchannel_args.ifx->count > 0) {
        ifx = app_wifi_interface_str2ifx(wifi_offchannel_args.ifx->sval[0]);
    }
    if (wifi_offchannel_args.mac->count > 0) {
        ret = wifi_cmd_str2mac(wifi_offchannel_args.mac->sval[0], mac);
        if (ret != ESP_OK) {
            ESP_LOGE(APP_TAG, "Can not parse mac: %s", wifi_offchannel_args.mac->sval[0]);
            return 1;
        }
    }
    if (wifi_offchannel_args.type->count > 0) {
        type = wifi_offchannel_args.type->ival[0];
    }
    if (wifi_offchannel_args.channel->count > 0) {
        channel = wifi_offchannel_args.channel->ival[0];
    }
    if (wifi_offchannel_args.number->count > 0) {
        count = wifi_offchannel_args.number->ival[0];
    }
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 5, 2)  /* 44a66c7521832fd5f6b907c30b6e1ec3924f9baf */
    if (wifi_offchannel_args.sec_channel->count > 0) {
        sec_channel = wifi_cmd_sec_channel_str2val(wifi_offchannel_args.sec_channel->sval[0]);
    }
#endif
    if (wifi_offchannel_args.wait_time_ms->count > 0) {
        wait_time_ms = wifi_offchannel_args.wait_time_ms->ival[0];
    }
    if (wifi_offchannel_args.no_ack->count > 0) {
        no_ack = true;
    }
    if (wifi_offchannel_args.op_id->count > 0) {
        op_id = wifi_offchannel_args.op_id->ival[0];
    }
    if (wifi_offchannel_args.data_len->count > 0) {
        data_len = wifi_offchannel_args.data_len->ival[0];
    }
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 0, 0)
    if (wifi_offchannel_args.bssid->count > 0) {
        ret = wifi_cmd_str2mac(wifi_offchannel_args.bssid->sval[0], bssid);
        if (ret != ESP_OK) {
            ESP_LOGE(APP_TAG, "Can not parse bssid: %s", wifi_offchannel_args.bssid->sval[0]);
            return 1;
        }
    }
    if (wifi_offchannel_args.allow_broadcast->count > 0) {
        allow_broadcast = true;
    }
#endif

    switch (action) {
    case WIFI_ACTION_OFFCHANNEL_TX:
        wifi_action_tx_req_t *req = (wifi_action_tx_req_t *)malloc(sizeof(wifi_action_tx_req_t) + data_len);
        if (req == NULL) {
            ESP_LOGE(APP_TAG, "malloc fail");
            return 1;
        }
        memset(req, 0, sizeof(wifi_action_tx_req_t) + data_len);
        memcpy(req->dest_mac, mac, 6);
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 0, 0)
        memcpy(req->bssid, bssid, 6);
#endif
        req->ifx = ifx;
        req->type = type;
        req->channel = channel;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 5, 2)  /* 44a66c7521832fd5f6b907c30b6e1ec3924f9baf */
        req->sec_channel = sec_channel;
#endif
        req->wait_time_ms = wait_time_ms;
        req->no_ack = no_ack;
        req->op_id = op_id;
        req->rx_cb = rx_cb;
        req->data_len = data_len;
        for (int i = 0; i < count; i++) {
            ret = esp_wifi_action_tx_req(req);
            vTaskDelay(esp_random() % 100 / portTICK_PERIOD_MS);
            if (ret != ESP_OK) {
                ESP_LOGE(APP_TAG, "esp_wifi_action_tx_req fail");
                break;
            }
        }
        free(req);
        rx_cb = NULL;
        LOG_WIFI_CMD_DONE(ret, "WIFI_OFFCHANNEL");
        break;
    case WIFI_ACTION_REMAIN_ON_CHANNEL:
        rx_cb = offchannel_rx_callback;
        wifi_roc_req_t roc = {0};
        roc.ifx = ifx;
        roc.type = type;
        roc.channel = channel;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 5, 2)  /* 44a66c7521832fd5f6b907c30b6e1ec3924f9baf */
        roc.sec_channel = sec_channel;
#endif
        roc.wait_time_ms = wait_time_ms;
        roc.op_id = op_id;
        roc.rx_cb = rx_cb;
        roc.done_cb = NULL;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 0, 0)
        roc.allow_broadcast = allow_broadcast;
#endif
        ret = esp_wifi_remain_on_channel(&roc);
        rx_cb = NULL;
        LOG_WIFI_CMD_DONE(ret, "WIFI_OFFCHANNEL");
        break;
    default:
        ESP_LOGE(APP_TAG, "Invalid action!");
        return 1;
    }
    return 0;
}

void wifi_cmd_register_wifi_offchannel(void)
{
    if (!wifi_offchannel_args.end) {
        wifi_offchannel_args.action = arg_str1(NULL, NULL, "<action>", "action: send|roc");
        wifi_offchannel_args.ifx = arg_str0("i", "interface", "<interface>", "interface: sta|ap");
        wifi_offchannel_args.mac = arg_str0("m", "mac", "<str>", "mac address 11:22:33:44:55:66");
        wifi_offchannel_args.type = arg_int0(NULL, "type", "<type>", "type: 0|1");
        wifi_offchannel_args.channel = arg_int0("n", "channel", "<int>", "channel");
        wifi_offchannel_args.number = arg_int0(NULL, "number", "<int>", "tx number");
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 5, 2)  /* 44a66c7521832fd5f6b907c30b6e1ec3924f9baf */
        wifi_offchannel_args.sec_channel = arg_str0("s", "sec-channel", "<str>", "second channel:none|above|below");
#endif
        wifi_offchannel_args.wait_time_ms = arg_int0(NULL, "wait-time", "<int>", "wait time in ms");
        wifi_offchannel_args.no_ack = arg_lit0(NULL, "no-ack", "no ack.");
        wifi_offchannel_args.op_id = arg_int0(NULL, "op-id", "<op_id>", "operation id");
        wifi_offchannel_args.data_len = arg_int0("l", "len", "<data_len>", "data length");
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 0, 0)
        wifi_offchannel_args.bssid = arg_str0(NULL, "bssid", "<str>", "bssid 11:22:33:44:55:66");
        wifi_offchannel_args.allow_broadcast = arg_lit0(NULL, "allow-broadcast", "allow broadcast");
#endif
        wifi_offchannel_args.end = arg_end(2);
    }
    const esp_console_cmd_t wifi_offchannel_cmd = {
        .command = "wifi_offchannel",
        .help = "offchannel tx or remain on channel <esp_wifi_offchannel_tx() or esp_wifi_remain_on_channel()>",
        .hint = NULL,
        .func = &cmd_do_wifi_offchannel,
        .argtable = &wifi_offchannel_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&wifi_offchannel_cmd));
}
#endif /* WIFI_CMD_OFFCHANNEL_SUPPORTED */
