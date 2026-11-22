/* =========================================================================
 * NODE 1: POMPA AIR DC (ONOFF SERVER)
 * Mata Kuliah : Desain dan Aplikasi IoT - ITS Surabaya
 * Peran Mesh  : Generic OnOff Server (Wajib Siaga / Always Listening)
 * Hardware    : ESP32 DevKit, Modul Relay 1-Channel 5V, Pompa Air DC 3-5V
 * ========================================================================= */

#include <Arduino.h>
#include <esp_ble_mesh_defs.h>
#include <esp_ble_mesh_common_api.h>
#include <esp_ble_mesh_networking_api.h>
#include <esp_ble_mesh_provisioning_api.h>
#include <esp_ble_mesh_generic_model_api.h>
#include <esp_ble_mesh_local_data_operation_api.h>
#include "mesh_config.h"

// State pompa (0 = MATI, 1 = HIDUP)
static uint8_t pump_state = 0;

// Deklarasi fungsi helper
void set_pump(uint8_t state, const char* source);
void init_ble_mesh();

// Struktur Model Generic OnOff Server
static esp_ble_mesh_gen_onoff_srv_t onoff_server = {
    .rsp_ctrl = {
        .get_auto_rsp = ESP_BLE_MESH_SERVER_AUTO_RSP,
        .set_auto_rsp = ESP_BLE_MESH_SERVER_AUTO_RSP,
    },
};

ESP_BLE_MESH_MODEL_PUB_DEFINE(onoff_srv_pub, 2 + 1, ROLE_NODE);

static esp_ble_mesh_model_t root_models[] = {
    ESP_BLE_MESH_MODEL_CFG_SRV(),
    ESP_BLE_MESH_MODEL_GEN_ONOFF_SRV(&onoff_srv_pub, &onoff_server),
};

static esp_ble_mesh_elem_t elements[] = {
    ESP_BLE_MESH_ELEMENT(0, root_models, ESP_BLE_MESH_MODEL_NONE),
};

static esp_ble_mesh_comp_t composition = {
    .cid = CID_ESP,
    .elements = elements,
    .element_count = ARRAY_SIZE(elements),
};

// Konfigurasi Provisioning
static esp_ble_mesh_prov_t prov = {
    .uuid = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
              0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x01 }, // Node 1 UUID
};

// Callback saat ada event provisioning
static void ble_mesh_provisioning_cb(esp_ble_mesh_prov_cb_event_t event,
                                     esp_ble_mesh_prov_cb_param_t *param) {
    switch (event) {
    case ESP_BLE_MESH_NODE_PROV_COMPLETE_EVT:
        Serial.printf("[NODE 1] Provisioning Selesai! Unicast Addr: 0x%04x, NetKey Idx: %d\n",
                      param->node_prov_complete.addr, param->node_prov_complete.net_idx);
        break;
    case ESP_BLE_MESH_NODE_PROV_RESET_EVT:
        Serial.println("[NODE 1] Node di-reset dari jaringan Mesh.");
        break;
    default:
        break;
    }
}

// Callback saat menerima pesan Generic OnOff Server
static void ble_mesh_generic_server_cb(esp_ble_mesh_generic_server_cb_event_t event,
                                       esp_ble_mesh_generic_server_cb_param_t *param) {
    switch (event) {
    case ESP_BLE_MESH_GENERIC_SERVER_STATE_CHANGE_EVT:
        if (param->ctx.recv_op == ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET ||
            param->ctx.recv_op == ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK) {
            uint8_t new_state = param->value.state_change.onoff_set.onoff;
            set_pump(new_state, "BLE Mesh Command");
        }
        break;
    case ESP_BLE_MESH_GENERIC_SERVER_RECV_GET_MSG_EVT:
        Serial.println("[NODE 1] Menerima permintaan GET status dari mesh.");
        break;
    default:
        break;
    }
}

void set_pump(uint8_t state, const char* source) {
    pump_state = state ? 1 : 0;
    // Modul relay 5V aktif HIGH / LOW tergantung modul. Umumnya aktif LOW (relay ON = LOW).
    // Disesuaikan: jika HIGH mengaktifkan pompa:
    digitalWrite(PIN_RELAY_PUMP, pump_state ? HIGH : LOW);
    digitalWrite(PIN_NODE1_LED, pump_state ? HIGH : LOW);

    Serial.println("==================================================");
    Serial.printf("[NODE 1 - POMPA] Status Pompa: %s (Sumber: %s)\n",
                  pump_state ? ">>> MENYALA (ON) <<<" : ">>> PADAM (OFF) <<<", source);
    Serial.printf("[NODE 1 - POMPA] Pin GPIO %d Level: %s\n",
                  PIN_RELAY_PUMP, pump_state ? "HIGH" : "LOW");
    Serial.println("==================================================");
}

void init_ble_mesh() {
    esp_err_t err;

    err = esp_ble_mesh_register_prov_cb(ble_mesh_provisioning_cb);
    if (err != ESP_OK) {
        Serial.printf("[NODE 1] Gagal register prov callback: %d\n", err);
        return;
    }

    err = esp_ble_mesh_register_custom_model_cb(NULL);
    err = esp_ble_mesh_register_generic_server_callback(ble_mesh_generic_server_cb);
    if (err != ESP_OK) {
        Serial.printf("[NODE 1] Gagal register server callback: %d\n", err);
        return;
    }

    err = esp_ble_mesh_init(&prov, &composition);
    if (err != ESP_OK) {
        Serial.printf("[NODE 1] Gagal inisialisasi BLE Mesh: %d\n", err);
        return;
    }

    Serial.println("[NODE 1] BLE Mesh Generic OnOff Server Berhasil Dimulai!");
    Serial.println("[NODE 1] Node wajib siaga mendengar perintah On/Off dari Node 2 / Node 4.");
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n--------------------------------------------------");
    Serial.println("DESIOT ITS - NODE 1: POMPA AIR DC (ONOFF SERVER)");
    Serial.println("--------------------------------------------------");

    // Konfigurasi Pin Relay dan LED
    pinMode(PIN_RELAY_PUMP, OUTPUT);
    pinMode(PIN_NODE1_LED, OUTPUT);
    digitalWrite(PIN_RELAY_PUMP, LOW); // Awal mati
    digitalWrite(PIN_NODE1_LED, LOW);

    // Inisialisasi Stack Bluetooth & Mesh
    init_ble_mesh();

    Serial.println("[NODE 1] Tip Debug: Ketik '1' (ON) atau '0' (OFF) di Serial Monitor untuk uji manual relay.");
}

void loop() {
    // Penanganan perintah manual melalui Serial Monitor untuk keperluan uji coba praktikum
    if (Serial.available()) {
        char ch = Serial.read();
        if (ch == '1') {
            set_pump(1, "Serial Manual Override");
        } else if (ch == '0') {
            set_pump(0, "Serial Manual Override");
        }
    }
    delay(50);
}
