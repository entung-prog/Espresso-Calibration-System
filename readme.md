# ☕ Coffee Calibration System

> Sistem Pengukuran Padatan Terlarut (TDS) dan pH Kopi Berbasis ESP32 dengan Algoritma Fuzzy Mamdani untuk Standarisasi Kalibrasi Espresso

---

# 📖 Deskripsi

Coffee Calibration System merupakan sistem berbasis ESP32 yang digunakan untuk membantu barista melakukan kalibrasi espresso menggunakan beberapa parameter.

Sistem mampu membaca:

- TDS (Total Dissolved Solid)
- pH Kopi
- Suhu Kopi

Kemudian data tersebut diproses menggunakan Algoritma Fuzzy Mamdani sehingga menghasilkan status ekstraksi seperti:

- Under Extract
- Ideal
- Over Extract

Sistem dapat digunakan oleh berbagai coffee shop karena seluruh parameter standar dapat dikustomisasi.

---

# 🎯 Tujuan

- Membuat alat kalibrasi espresso berbasis IoT
- Membantu standarisasi kualitas espresso
- Menyediakan dashboard monitoring realtime
- Menyimpan histori kalibrasi
- Mendukung banyak profil cafe

---

# 🏗 Arsitektur Sistem

```

Sensor TDS
│
Sensor pH
│
Sensor Suhu
│
▼

ESP32

│

REST API + WebSocket

│

Next.js Dashboard

│

PostgreSQL

```

---

# ⚙ Hardware

## ESP32

- ESP32 DOIT DevKit V1

## Sensor

- Gravity Analog TDS Meter V1.0
- E-201-C pH Sensor
- DS18B20 Waterproof

---

# 💻 Software

## Firmware

- PlatformIO
- Arduino Framework

## Dashboard

- Next.js
- TypeScript
- Tailwind CSS
- ShadCN UI
- Chart.js

## Backend

- Next.js API
- Prisma ORM
- PostgreSQL

---

# 📂 Struktur Project

```

coffee-calibration/

firmware/
esp32/

dashboard/

backend/

database/

docs/

```

---

# 📊 Dashboard

## 1 Dashboard

Menampilkan data realtime.

### Card

- pH
- TDS
- Suhu
- Status

---

## 2 Realtime Monitoring

Grafik

- TDS
- pH
- Suhu

Realtime menggunakan WebSocket.

---

## 3 Calibration

Digunakan untuk menentukan standar cafe.

Parameter:

- TDS Minimum
- TDS Maximum
- pH Minimum
- pH Maximum
- Temperatur Minimum
- Temperatur Maximum

---

## 4 Cafe Profile

Setiap cafe memiliki standar berbeda.

Contoh:

Coffee Lab

- TDS 8.5–9.5
- pH 5.1–5.4

Cafe B

- TDS 9.2–10
- pH 5.0–5.3

Admin dapat:

- Tambah Cafe
- Edit Cafe
- Hapus Cafe

---

## 5 History

Menyimpan seluruh hasil pengukuran.

Data:

- Tanggal
- Jam
- Nama Cafe
- TDS
- pH
- Suhu
- Status

Export:

- CSV
- Excel

---

## 6 Analytics

Grafik:

- TDS Harian
- pH Harian
- Suhu
- Status Kalibrasi

---

## 7 Sensor Calibration

Kalibrasi Sensor pH

- Buffer pH 4
- Buffer pH 6.86
- Buffer pH 9.18

Kalibrasi TDS

- 342 ppm
- 707 ppm
- 1000 ppm

---

# ☕ Coffee Profile

Setiap cafe memiliki profile.

```

Cafe

↓

Nama

↓

Target TDS

↓

Target pH

↓

Target Temperature

↓

Tolerance

↓

Save

```

---

# 🧠 Fuzzy Mamdani

Input

## TDS

- Rendah
- Ideal
- Tinggi

## pH

- Asam
- Ideal
- Basa

## Temperatur

- Rendah
- Ideal
- Tinggi

Output

- Under Extract
- Ideal Espresso
- Over Extract

---

# 📡 API

## Sensor

GET

/api/sensor

Response

```json
{
  "temperature": 91.2,
  "ph": 5.21,
  "tds": 9.12
}
```

---

## Calibration

GET

```
/api/calibration
```

POST

```
/api/calibration
```

PUT

```
/api/calibration/:id
```

---

## History

GET

```
/api/history
```

---

## Cafe

GET

```
/api/cafe
```

POST

```
/api/cafe
```

PUT

```
/api/cafe/:id
```

DELETE

```
/api/cafe/:id
```

---

# 🗄 Database

## Cafe

```
id
name
createdAt
updatedAt
```

---

## CalibrationSetting

```
id
cafeId

tdsMin
tdsMax

phMin
phMax

tempMin
tempMax

tolerance
```

---

## Measurement

```
id
cafeId

temperature
ph
tds

status

createdAt
```

---

# 🌐 Fitur Tambahan

- Dark Mode
- Export CSV
- Export Excel
- Responsive
- Login Admin
- Multi Cafe
- Auto Refresh
- OTA Update ESP32
- Backup Database

---

# Dashboard Web

Dashboard Next.js sudah dibuat di folder `dashboard` dan siap deploy ke Vercel.

Isi utamanya:

- Realtime monitoring dari endpoint ESP32 `/api/sensor`
- Calibration form untuk update standar TDS, pH, dan suhu
- Cafe profile
- History measurement dengan PostgreSQL
- Export CSV

Lihat panduan lengkap di `dashboard/README.md`.

---

# 🚀 Tahapan Pengembangan

## Implementasi Firmware Saat Ini

File firmware utama ada di `src/main.cpp` dan sudah mencakup:

- Pembacaan sensor TDS, pH, dan DS18B20
- Kompensasi suhu untuk pembacaan TDS
- REST API sensor langsung dari ESP32
- Mode Access Point fallback jika SSID WiFi belum diisi atau koneksi gagal

Logic Fuzzy Mamdani dan setting calibration dipindahkan ke backend dashboard
Next.js agar lebih mudah dikembangkan untuk banyak cafe dan histori jangka
panjang.

### Konfigurasi WiFi

Ubah nilai berikut di `src/main.cpp`:

```cpp
const char *WIFI_SSID = "";
const char *WIFI_PASSWORD = "";
```

Jika tetap kosong, ESP32 membuat access point:

```text
SSID     : Espresso-Calibrator
Password : espresso123
```

### Endpoint Firmware

```http
GET /api/sensor
```

Contoh response:

```json
{
  "temperature": 91.2,
  "ph": 5.21,
  "tds": 9.12,
  "tdsPpm": 912
}
```

Status ekstraksi dihasilkan oleh backend dashboard melalui endpoint
`/api/evaluate`.

## Phase 1

- ESP32
- Sensor TDS
- Sensor pH
- DS18B20

---

## Phase 2

REST API ESP32

---

## Phase 3

Dashboard Next.js

---

## Phase 4

Database PostgreSQL

---

## Phase 5

Fuzzy Mamdani

---

## Phase 6

History

---

## Phase 7

Analytics

---

## Phase 8

Deployment

---

# 📈 Pengembangan Selanjutnya

- Multi-user (Admin, Barista, Owner)
- Integrasi QR Code untuk tiap mesin espresso
- Kalibrasi otomatis berdasarkan histori
- Prediksi kualitas espresso menggunakan Machine Learning
- Sinkronisasi cloud untuk banyak cabang kafe
