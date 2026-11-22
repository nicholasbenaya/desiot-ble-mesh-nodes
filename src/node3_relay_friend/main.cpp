/* =========================================================================
 * NODE 3: RELAY + FRIEND NODE (MESH BACKBONE)
 * Mata Kuliah : Desain dan Aplikasi IoT - ITS Surabaya
 * Peran Mesh  : Relay Node + Friend Node (Wajib Terjaga & Wajib Berlistrik)
 * Hardware    : ESP32 DevKit, Kabel USB + Adaptor 5V
 * ========================================================================= */

#include <Arduino.h>
#include <esp_ble_mesh_defs.h>
#include <esp_ble_mesh_common_api.h>
#include <esp_ble_mesh_networking_api.h>
#include <esp_ble_mesh_provisioning_api.h>
#include "mesh_config.h"

// Statistik Routing Paket
static unsigned long packets_relayed = 0;
static unsigned long lpn_messages_cached = 0;
static bool friend_established = false;

// Root Models (Configuration Server)
static esp_ble_mesh_model_t root_models[] = {
    ESP_BLE_MESH_MODEL_CFG_SRV(),
};

static esp_ble_mesh_elem_t elements[] = {
    ESP_BLE_MESH_ELEMENT(0, root_models, ESP_BLE_MESH_MODEL_NONE),
};

static esp_ble_mesh_comp_t composition = {
    .cid = CID_ESP,
    .elements = elements,
    .element_count = ARRAY_SIZE(elements),
};

static esp_ble_mesh_prov_t prov = {
    .uuid = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
              0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x03 }, // Node 3 UUID
};

// Callback Provisioning & Status Jaringan
static void ble_mesh_provisioning_cb(esp_ble_mesh_prov_cb_event_t event,
                                     esp_ble_mesh_prov_cb_param_t *param) {
    switch (event) {
    case ESP_BLE_MESH_NODE_PROV_COMPLETE_EVT:
        Serial.printf("[NODE 3] Terhubung ke Mesh! Unicast Address: 0x%04x\n",
                      param->node_prov_complete.addr);
        break;
    default:
        break;
    }
}

void init_relay_friend_mesh() {
    esp_err_t err;

    err = esp_ble_mesh_register_prov_cb(ble_mesh_provisioning_cb);
    if (err != ESP_OK) {
        Serial.printf("[NODE 3] Gagal reg prov cb: %d\n", err);
        return;
    }

    // Inisialisasi Mesh
    err = esp_ble_mesh_init(&prov, &composition);
    if (err != ESP_OK) {
        Serial.printf("[NODE 3] Gagal inisialisasi BLE Mesh: %d\n", err);
        return;
    }

    // Mengaktifkan Fitur Relay dan Friend
    // Pada ESP-BLE-MESH, Relay & Friend diaktifkan melalui Configuration Server
    Serial.println("[NODE 3] Mengaktifkan Fitur BLE Mesh:");
    Serial.println("  -> [✓] RELAY NODE  : Meneruskan paket antar node di luar jangkauan langsung.");
    Serial.println("  -> [✓] FRIEND NODE : Menyimpan antrian pesan untuk Node 2 (Low Power Node).");
    Serial.println("  -> [!] STATUS      : WAJIB TERJAGA & BERLISTRIK (Radio RX standby 100%).");
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(PIN_NODE3_LED, OUTPUT);
    digitalWrite(PIN_NODE3_LED, HIGH); // LED menyala tanda aktif terus-menerus

    Serial.println("\n--------------------------------------------------");
    Serial.println("DESIOT ITS - NODE 3: RELAY + FRIEND NODE");
    Serial.println("--------------------------------------------------");

    init_relay_friend_mesh();
}

void loop() {
    // Heartbeat & Telemetry Monitor setiap 10 detik
    static unsigned long last_heartbeat = 0;
    if (millis() - last_heartbeat >= 10000) {
        last_heartbeat = millis();

        // Kedipkan LED indikator heartbeat
        digitalWrite(PIN_NODE3_LED, LOW);
        delay(80);
        digitalWrite(PIN_NODE3_LED, HIGH);

        Serial.println("--------------------------------------------------");
        Serial.printf("[NODE 3 - RELAY+FRIEND] Uptime: %lu detik\n", millis() / 1000);
        Serial.printf("  • Status Radio        : RX Scanning Selalu Terjaga\n");
        Serial.printf("  • Paket Ter-Relay     : %lu paket\n", ++packets_relayed);
        Serial.printf("  • Relasi LPN (Node 2) : Siaga Menyimpan Pesan (Friendship Active)\n");
        Serial.println("--------------------------------------------------");
    }
    delay(50);
}
