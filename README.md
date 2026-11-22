# DESIOT BLE Mesh & MQTT Gateway Irrigation System

Proyek Praktikum **Desain dan Aplikasi IoT (DESIOT)** - Departemen Teknik Komputer / Teknik Elektro, **Institut Teknologi Sepuluh Nopember (ITS)** Surabaya.

---

## 📌 Arsitektur Topologi Sistem 4-Node

```text
[ Node 2: Sensor Kelembapan ]
       (Low Power Node)
              |
         (BLE Mesh)
              v
[ Node 3: Relay + Friend ] <====== (BLE Mesh) ======> [ Node 1: Pompa DC ]
  (Wajib Siaga / Listrik)                               (OnOff Server)
              |
         (BLE Mesh)
              v
[ Node 4: Provisioner + Proxy + Gateway Wi-Fi ]
              |
         (Wi-Fi MQTT)
              v
[ Komputer Host: Eclipse Mosquitto Broker + Visualisasi Grafik ]
```

---

## 📂 Struktur Direktori Proyek

```text
firmware-nodes/
├── include/
│   └── mesh_config.h         # Konfigurasi bersama (NetKey, AppKey, Pinout, Topik MQTT)
├── src/
│   ├── node1_pump/           # Firmware Node 1: OnOff Server pengendali pompa DC
│   │   └── main.cpp
│   ├── node2_sensor_lpn/     # Firmware Node 2: Low Power Node (LPN) pembaca sensor tanah
│   │   └── main.cpp
│   ├── node3_relay_friend/   # Firmware Node 3: Relay & Friend Node penerus paket mesh
│   │   └── main.cpp
│   └── node4_gateway/        # Firmware Node 4: Gateway BLE Mesh ke Wi-Fi & MQTT
│       └── main.cpp
├── platformio.ini             # Konfigurasi multi-environment PlatformIO
├── .gitignore
└── README.md
```

---

## 🔌 Detail Hardware & Pinout Tiap Node

| Node | Peran Mesh | Hardware Utama | Pinout ESP32 | Status Daya |
| :---: | :--- | :--- | :--- | :--- |
| **Node 1** | Generic OnOff Server | Pompa DC 3-5V, Modul Relay 5V | `GPIO 23` ➔ IN Relay<br>`VIN (5V)` ➔ VCC & COM<br>`NO Relay` ➔ (+) Pompa | Wajib Siaga (Adaptor USB 5V) |
| **Node 2** | Low Power Node (LPN) | Capacitive Soil Moisture v1.2 | `GPIO 34` ➔ AOUT Sensor<br>`3V3` ➔ VCC Sensor<br>`VIN` ➔ (+) Baterai 9V/18650 | Baterai Mandiri (Deep Sleep) |
| **Node 3** | Relay + Friend Node | ESP32 DevKit | Radio RX standby 100% | Wajib Berlistrik (Adaptor USB 5V) |
| **Node 4** | Provisioner + Proxy + Gateway | ESP32 DevKit | Dual Radio (BLE Mesh + Wi-Fi) | Wajib Berlistrik (Kabel USB ke PC) |

---

## 🚀 Panduan Kompilasi & Upload (PlatformIO)

Buka terminal di dalam folder `firmware-nodes/`, lalu jalankan perintah upload sesuai node yang terhubung:

### 1. Upload ke Node 1 (Pompa DC)
```bash
pio run -e node1_pump -t upload
pio device monitor -b 115200
```

### 2. Upload ke Node 2 (Sensor Kelembapan LPN)
```bash
pio run -e node2_sensor_lpn -t upload
pio device monitor -b 115200
```

### 3. Upload ke Node 3 (Relay + Friend Node)
```bash
pio run -e node3_relay_friend -t upload
pio device monitor -b 115200
```

### 4. Upload ke Node 4 (Gateway Wi-Fi MQTT)
```bash
pio run -e node4_gateway -t upload
pio device monitor -b 115200
```

> **Tip:** Jika menggunakan extension PlatformIO di VS Code, Anda cukup memilih environment yang ingin di-upload melalui panel bawah PlatformIO (`env:node1_pump`, `env:node2_sensor_lpn`, dst.).

---

## 🖥️ Setup Eclipse Mosquitto Broker di Komputer Host

1. **Jalankan Broker Mosquitto (Verbose Mode):**
   ```bash
   mosquitto -v -p 1883
   ```
2. **Buka Terminal Baru untuk Memonitor Topik Sensor:**
   ```bash
   mosquitto_sub -h localhost -p 1883 -t "desiot/#" -v
   ```
3. **Uji Kontrol Manual Pompa dari Komputer:**
   ```bash
   # Nyalakan Pompa
   mosquitto_pub -h localhost -p 1883 -t "desiot/pump/command" -m "ON"

   # Matikan Pompa
   mosquitto_pub -h localhost -p 1883 -t "desiot/pump/command" -m "OFF"
   ```

---

## 👥 Matriks Klaim Pekerjaan Anggota Tim (ITS)

* **Anggota 1:** Koordinator Hardware & Node 1 (Pompa DC, Relay 5V, OnOff Server firmware) - **25%**
* **Anggota 2:** Spesialis Low Power & Node 2 (Sensor Kelembapan Kapasitif, kalibrasi ADC, LPN Deep Sleep) - **25%**
* **Anggota 3:** Spesialis Mesh & Node 3 (Relay Node, Friend Node, manajemen buffer antrian paket) - **25%**
* **Anggota 4:** Spesialis Gateway & Server Host (Dual-Radio BLE Proxy + Wi-Fi Gateway MQTT, setup Mosquitto) - **25%**
