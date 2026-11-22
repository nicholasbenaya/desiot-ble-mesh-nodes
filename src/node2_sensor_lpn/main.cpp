/* =========================================================================
 * NODE 2: SENSOR KELEMBAPAN TANAH (LOW POWER NODE / LPN)
 * Mata Kuliah : Desain dan Aplikasi IoT - ITS Surabaya
 * Peran Mesh  : Low Power Node (Tidur, Bangun Berkala, Friendship dg Node 3)
 * Hardware    : ESP32 DevKit, Capacitive Soil Moisture v1.2, Catu Baterai
 * ========================================================================= */

#include <Arduino.h>
#include <esp_sleep.h>
#include <esp_ble_mesh_defs.h>
#include <esp_ble_mesh_common_api.h>
#include <esp_ble_mesh_networking_api.h>
#include <esp_ble_mesh_provisioning_api.h>
#include <esp_ble_mesh_generic_model_api.h>
#include <esp_ble_mesh_low_power_api.h>
#include "mesh_config.h"

// Variabel di RTC Memory (tidak hilang saat deep sleep)
RTC_DATA_ATTR int boot_count = 0;
RTC_DATA_ATTR int last_moisture_raw = 0;

// Model Generic OnOff Client untuk mengirim perintah ke Pompa (Node 1)
static esp_ble_mesh_client_t onoff_client;

ESP_BLE_MESH_MODEL_PUB_DEFINE(onoff_cli_pub, 2 + 1, ROLE_NODE);

static esp_ble_mesh_model_t root_models[] = {
    ESP_BLE_MESH_MODEL_CFG_SRV(),
    ESP_BLE_MESH_MODEL_GEN_ONOFF_CLI(&onoff_cli_pub, &onoff_client),
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
              0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x02 }, // Node 2 UUID
};

// Fungsi membaca analog kelembapan dengan averaging filter
int read_soil_moisture() {
    long sum = 0;
    const int SAMPLES = 10;
    for (int i = 0; i < SAMPLES; i++) {
        sum += analogRead(PIN_SOIL_ANALOG);
        delay(10);
    }
    return (int)(sum / SAMPLES);
}

// Mengirim perintah aktuasi OnOff ke Node 1 melalui Mesh
void send_mesh_onoff_cmd(uint8_t state) {
    esp_ble_mesh_generic_client_set_state_t set_state;
    set_state.onoff_set.op_en = false;
    set_state.onoff_set.onoff = state;
    set_state.onoff_set.tid = (uint8_t)boot_count;

    esp_ble_mesh_client_common_param_t common = {
        .opcode = ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK,
        .model = &root_models[1],
        .ctx = {
            .net_idx = ESP_BLE_MESH_NET_IDX,
            .app_idx = ESP_BLE_MESH_APP_IDX,
            .addr = ADDR_PUMP_NODE, // Target: Node 1 (Pompa)
            .send_rel = false,
            .send_ttl = 3,
            .send_tag = 0,
        },
        .msg_timeout = 0,
        .role = ROLE_NODE,
    };

    esp_err_t err = esp_ble_mesh_generic_client_set_state(&common, &set_state);
    if (err == ESP_OK) {
        Serial.printf("[NODE 2 - LPN] Berhasil mengirim perintah OnOff: %d ke Node 1 (Pompa)\n", state);
    } else {
        Serial.printf("[NODE 2 - LPN] Mengirim paket mesh (code: %d)\n", err);
    }
}

// Konfigurasi Low Power Node (LPN) & Deep Sleep
void enter_deep_sleep() {
    Serial.println("[NODE 2 - LPN] Menyiapkan mode Deep Sleep untuk menghemat baterai...");
    Serial.printf("[NODE 2 - LPN] ESP32 akan tidur selama %d detik.\n", LPN_SLEEP_SEC);
    Serial.println("==================================================\n");
    Serial.flush();

    // Set timer wake-up dalam mikrodetik
    esp_sleep_enable_timer_wakeup((uint64_t)LPN_SLEEP_SEC * 1000000ULL);
    esp_deep_sleep_start();
}

void setup() {
    Serial.begin(115200);
    delay(500);

    boot_count++;
    Serial.println("\n--------------------------------------------------");
    Serial.printf("DESIOT ITS - NODE 2: SENSOR KELEMBAPAN (LOW POWER NODE)\n");
    Serial.printf("Siklus Bangun (Boot Count): #%d\n", boot_count);
    Serial.println("--------------------------------------------------");

    // 1. Konfigurasi ADC Pin Sensor Kelembapan
    pinMode(PIN_SOIL_ANALOG, INPUT);
    analogReadResolution(12); // Resolusi 12-bit (0 - 4095)

    // 2. Baca Sensor Kelembapan Tanah
    int moisture_raw = read_soil_moisture();
    last_moisture_raw = moisture_raw;

    // Evaluasi Kondisi Tanah
    const char* status_tanah;
    uint8_t command_pompa = 0;

    if (moisture_raw >= SOIL_DRY_THRESHOLD) {
        status_tanah = "KERING (Butuh Penyiraman)";
        command_pompa = 1; // Trigger Pompa ON
    } else if (moisture_raw <= SOIL_WET_THRESHOLD) {
        status_tanah = "BASAH (Cukup Air)";
        command_pompa = 0; // Pompa OFF
    } else {
        status_tanah = "OPTIMAL (Normal)";
        command_pompa = 0;
    }

    Serial.printf("[NODE 2 - LPN] Nilai ADC Kelembapan : %d / 4095\n", moisture_raw);
    Serial.printf("[NODE 2 - LPN] Status Kondisi Tanah: %s\n", status_tanah);
    Serial.printf("[NODE 2 - LPN] Rekomendasi Pompa    : %s\n", command_pompa ? "AKTIFKAN POMPA" : "MATIKAN POMPA");

    // 3. Inisialisasi Stack BLE Mesh & Kirim Data
    esp_ble_mesh_init(&prov, &composition);
    delay(200); // Beri waktu stack radio

    // 4. Kirim Instruksi ke Node 1 (Pompa) & Gateway
    send_mesh_onoff_cmd(command_pompa);
    delay(300); // Waktu transmisi radio sebelum radio dimatikan

    // 5. Masuk kembali ke Deep Sleep
    enter_deep_sleep();
}

void loop() {
    // Tidak pernah dieksekusi karena ESP32 langsung masuk deep sleep di setup()
}
