# Tutorial Setup Espresso Calibration System

Panduan ini menjelaskan cara merakit alat dari awal, meng-upload kode ke ESP32, membaca data sensor, lalu menjalankan dashboard web.

Project ini terdiri dari dua bagian:

- Firmware ESP32: file utama ada di `src/main.cpp`
- Dashboard web: ada di folder `dashboard`

## 1. Hardware yang Dibutuhkan

- ESP32 DOIT DevKit V1
- Sensor TDS analog, contoh Gravity Analog TDS Meter
- Sensor pH analog, contoh modul pH probe E-201-C
- Sensor suhu DS18B20 waterproof
- Resistor 4.7k ohm untuk DS18B20
- Kabel jumper
- Kabel USB data untuk ESP32
- Laptop/PC dengan Visual Studio Code

Catatan penting:

- ESP32 hanya aman menerima tegangan analog maksimal 3.3V di pin ADC.
- Jika modul TDS atau pH diberi 5V, pastikan output analognya tidak melebihi 3.3V sebelum masuk ke ESP32.
- Semua GND harus disambungkan jadi satu.

## 2. Pin Wiring ESP32

Kode firmware saat ini memakai pin berikut:

| Komponen | Pin modul sensor | Pin ESP32 |
| --- | --- | --- |
| Sensor TDS | VCC | 3V3 |
| Sensor TDS | GND | GND |
| Sensor TDS | AOUT / Analog Out | GPIO34 |
| Sensor pH | VCC | 3V3 |
| Sensor pH | GND | GND |
| Sensor pH | PO / Analog Out | GPIO32 |
| DS18B20 | VCC merah | 3V3 |
| DS18B20 | GND hitam | GND |
| DS18B20 | DATA kuning | GPIO4 |

Tambahkan resistor 4.7k ohm untuk DS18B20:

- Satu kaki resistor ke DATA DS18B20 / GPIO4
- Satu kaki resistor ke 3V3

Skema sederhana:

```text
ESP32 3V3  -> VCC TDS, VCC pH, VCC DS18B20
ESP32 GND  -> GND TDS, GND pH, GND DS18B20
ESP32 34   -> Analog Out TDS
ESP32 32   -> Analog Out pH
ESP32 4    -> DATA DS18B20

Resistor 4.7k:
GPIO4/DATA DS18B20 -> 3V3
```

## 3. Install Software

1. Install Visual Studio Code.
2. Install extension PlatformIO IDE di VS Code.
3. Install Node.js LTS jika ingin menjalankan dashboard web.
4. Buka folder project ini di VS Code:

```text
C:\Users\Entung\Documents\PlatformIO\Projects\espresso
```

## 4. Cek Konfigurasi PlatformIO

File `platformio.ini` sudah disiapkan untuk ESP32 DOIT DevKit V1:

```ini
[env:esp32doit-devkit-v1]
platform = espressif32
board = esp32doit-devkit-v1
framework = arduino
monitor_speed = 115200
lib_deps =
    paulstoffregen/OneWire@^2.3.8
    milesburton/DallasTemperature@^4.0.6
```

Library sensor akan otomatis di-download oleh PlatformIO saat build pertama.

## 5. Setting WiFi ESP32

Buka file:

```text
src/main.cpp
```

Cari bagian ini:

```cpp
const char *DEFAULT_WIFI_SSID = "";
const char *DEFAULT_WIFI_PASSWORD = "";
```

Ada dua pilihan. Firmware sekarang juga menyimpan kredensial WiFi di Preferences, jadi kalau mau ganti tanpa rebuild kamu bisa kirim `POST /api/wifi` ke ESP32.

### Pilihan A: Pakai Access Point bawaan ESP32

Biarkan kosong seperti ini:

```cpp
const char *DEFAULT_WIFI_SSID = "";
const char *DEFAULT_WIFI_PASSWORD = "";
```

Nanti ESP32 akan membuat WiFi sendiri:

```text
Nama WiFi : Espresso-Calibrator
Password  : espresso123
IP ESP32  : 192.168.4.1
```

### Pilihan B: ESP32 masuk ke WiFi rumah/kafe

Isi nama WiFi dan password:

```cpp
const char *DEFAULT_WIFI_SSID = "NAMA_WIFI";
const char *DEFAULT_WIFI_PASSWORD = "PASSWORD_WIFI";
```

Setelah berhasil connect, IP ESP32 akan tampil di Serial Monitor.

## 6. Upload Kode ke ESP32

1. Colok ESP32 ke laptop memakai kabel USB data.
2. Buka project ini di VS Code.
3. Klik ikon PlatformIO di sidebar kiri.
4. Pilih `PROJECT TASKS`.
5. Pilih environment `esp32doit-devkit-v1`.
6. Klik `Build` untuk cek compile.
7. Klik `Upload` untuk memasukkan kode ke ESP32.

Alternatif lewat terminal PlatformIO:

```powershell
pio run
pio run --target upload
```

Jika upload gagal dengan pesan sulit connect:

1. Tekan dan tahan tombol `BOOT` di ESP32.
2. Jalankan upload lagi.
3. Lepas tombol `BOOT` saat terminal mulai menulis `Writing at...`.
4. Tekan tombol `EN` atau `RST` setelah upload selesai.

## 7. Buka Serial Monitor

Setelah upload selesai, buka Serial Monitor:

```powershell
pio device monitor
```

Baud rate yang dipakai adalah `115200`.

Output yang benar kira-kira seperti ini:

```text
ESPRESSO SENSOR NODE
REST API: /api/sensor
Temperature : 25.00 C
TDS         : 0.00 % / 0 ppm
PH          : 7.00
```

Jika mode Access Point aktif, Serial Monitor akan menampilkan:

```text
AP SSID     : Espresso-Calibrator
AP IP       : 192.168.4.1
```

Jika ESP32 masuk WiFi router, Serial Monitor akan menampilkan IP dari router, misalnya:

```text
IP Address  : 192.168.1.25
```

## 8. Tes API Sensor ESP32

Jika memakai Access Point bawaan:

1. Sambungkan laptop/HP ke WiFi `Espresso-Calibrator`.
2. Masukkan password `espresso123`.
3. Buka browser:

```text
http://192.168.4.1/api/sensor
```

Jika ESP32 masuk WiFi router, ganti IP sesuai yang muncul di Serial Monitor:

```text
http://IP_ESP32/api/sensor
```

Contoh:

```text
http://192.168.1.25/api/sensor
```

Response normal:

```json
{
  "temperature": 25.00,
  "ph": 7.00,
  "tds": 0.00,
  "tdsPpm": 0,
  "raw": {
    "tdsAdc": 0,
    "phAdc": 3102,
    "phVoltage": 2.500
  }
}
```

## 9. Jalankan Dashboard Web

Masuk ke folder dashboard:

```powershell
cd dashboard
```

Install dependency:

```powershell
npm install
```

Buat file `.env.local` di folder `dashboard`.

Jika dashboard dipakai lokal dan ESP32 ada di jaringan yang sama:

```env
NEXT_PUBLIC_DEFAULT_DEVICE_URL="http://192.168.4.1"
```

Jika ESP32 masuk WiFi router, isi IP ESP32:

```env
NEXT_PUBLIC_DEFAULT_DEVICE_URL="http://192.168.1.25"
```

Kalau dashboard dideploy ke Vercel, kosongkan `NEXT_PUBLIC_DEFAULT_DEVICE_URL` atau pakai URL HTTPS publik/tunnel. Browser HTTPS tidak bisa fetch ke IP lokal ESP32 langsung.

Untuk database PostgreSQL, tambahkan:

```env
DATABASE_URL="postgresql://USER:PASSWORD@HOST:5432/DATABASE?sslmode=require"
```

Jika belum punya database, dashboard tetap bisa dibuka, tetapi fitur simpan cafe, kalibrasi, dan history tidak aktif penuh.

Jalankan dashboard:

```powershell
npm run dev
```

Buka browser:

```text
http://localhost:3000
```

## 10. Setup Database PostgreSQL

Langkah ini hanya perlu jika ingin menyimpan cafe, setting kalibrasi, dan history.

1. Siapkan PostgreSQL, misalnya dari Neon, Supabase, Railway, Vercel Postgres, atau PostgreSQL lokal.
2. Isi `DATABASE_URL` di `dashboard/.env.local`.
3. Jalankan migration:

```powershell
npm run prisma:deploy
```

Untuk development lokal, boleh memakai:

```powershell
npm run prisma:push
```

## 11. Alur Pemakaian

1. Nyalakan ESP32.
2. Pastikan sensor sudah tersambung sesuai tabel pin.
3. Pastikan laptop/HP satu jaringan dengan ESP32.
4. Cek `http://IP_ESP32/api/sensor`.
5. Jalankan dashboard dengan `npm run dev`.
6. Buka `http://localhost:3000`.
7. Masukkan URL device ESP32 jika dashboard menyediakan input device URL.
8. Pilih atau buat profil cafe.
9. Atur target TDS, pH, dan suhu.
10. Celupkan probe ke sampel espresso.
11. Tunggu data stabil.
12. Simpan hasil measurement jika database sudah aktif.

## 12. Kalibrasi Sensor

### Kalibrasi pH

Gunakan cairan buffer pH, misalnya pH 4.00, 6.86, dan 9.18.

Di firmware, rumus pH memakai nilai:

```cpp
const float PH_NEUTRAL_VOLTAGE = 2.50f;
const float PH_SLOPE = 0.18f;
```

Jika pembacaan pH meleset, sesuaikan dua nilai tersebut berdasarkan hasil buffer.

Langkah sederhana:

1. Celupkan probe ke buffer pH 6.86 atau 7.00.
2. Lihat `raw.phVoltage` dari endpoint `/api/sensor`.
3. Pakai nilai voltage itu sebagai acuan `PH_NEUTRAL_VOLTAGE`.
4. Cek lagi dengan buffer pH 4.00 dan 9.18.
5. Sesuaikan `PH_SLOPE` sampai hasil mendekati nilai buffer.

### Kalibrasi TDS

Gunakan larutan standar TDS, misalnya 342 ppm, 707 ppm, atau 1000 ppm.

Firmware menghitung TDS dari voltage dan kompensasi suhu. Jika hasil berbeda jauh:

1. Pastikan sensor TDS diberi supply sesuai modul.
2. Pastikan output analog tidak lebih dari 3.3V.
3. Celupkan probe ke larutan standar.
4. Bandingkan nilai `tdsPpm` dengan nilai larutan.
5. Jika perlu, tambahkan faktor koreksi di rumus `tdsPpm` pada `src/main.cpp`.

## 13. Troubleshooting

### ESP32 tidak terdeteksi

- Coba kabel USB lain yang mendukung data.
- Install driver USB to Serial sesuai chip board, biasanya CP210x atau CH340.
- Cek Device Manager untuk melihat port COM.

### Upload gagal

- Tekan tombol `BOOT` saat proses upload mulai.
- Tutup Serial Monitor sebelum upload.
- Coba port USB lain.
- Pastikan board di `platformio.ini` adalah `esp32doit-devkit-v1`.

### API tidak bisa dibuka

- Pastikan ESP32 menyala.
- Pastikan laptop/HP satu jaringan dengan ESP32.
- Jika mode AP, sambungkan ke WiFi `Espresso-Calibrator`.
- Cek IP ESP32 dari Serial Monitor.
- Buka endpoint lengkap: `http://IP_ESP32/api/sensor`.

### Dashboard tidak membaca sensor

- Pastikan `NEXT_PUBLIC_DEFAULT_DEVICE_URL` benar untuk mode yang dipakai.
- Untuk dashboard Vercel, jangan arahkan ke IP lokal ESP32; gunakan dashboard lokal atau URL HTTPS publik/tunnel.
- Coba buka API sensor langsung di browser saat debugging lokal.
- Jika pakai hotspot/tethering, pastikan hotspot 2.4 GHz dan WPA2; ESP32 tidak bisa join 5 GHz atau WPA3.

### Nilai sensor aneh

- Pastikan GND semua modul tersambung ke GND ESP32.
- Jangan biarkan pin analog menggantung tanpa sensor.
- Pastikan output analog sensor tidak melebihi 3.3V.
- Kalibrasi pH dan TDS dengan cairan standar.
- Jauhkan kabel sensor dari sumber noise seperti adaptor buruk atau motor.

## 14. File Penting

```text
platformio.ini          Konfigurasi board ESP32 dan library
src/main.cpp            Firmware ESP32
dashboard/              Dashboard Next.js
dashboard/.env.local    Konfigurasi URL ESP32 dan database
dashboard/prisma/       Schema dan migration database
```

## 15. Endpoint yang Dipakai

Firmware ESP32 menyediakan endpoint:

```http
GET /api/sensor
```

Dashboard membaca endpoint itu, lalu mengevaluasi status ekstraksi memakai backend dashboard:

```http
POST /api/evaluate
```

Status seperti under extract, ideal, atau over extract dihitung di dashboard dan juga tersedia dari firmware ESP32 lewat `/api/sensor`.
