# Espresso Calibration System

Panduan lengkap untuk merakit ESP32 sensor node, menjalankan dashboard lokal, menghubungkan Neon PostgreSQL, dan menangani skenario error jaringan yang umum terjadi.

Project ini punya dua bagian utama:

- Firmware ESP32: `src/main.cpp`
- Dashboard web Next.js: `dashboard/`

## Ringkasan Sistem

ESP32 membaca sensor TDS, pH, dan suhu. Dashboard membaca data ESP32 lewat HTTP, mengevaluasi status ekstraksi, lalu bisa menyimpan cafe, standar kalibrasi, dan history ke PostgreSQL/Neon. Setiap cafe bisa memiliki standar espresso sendiri berdasarkan shot yang dianggap paling tepat oleh barista.

Alur data normal:

```text
Sensor -> ESP32 -> Dashboard lokal/Vercel -> Neon PostgreSQL
```

Endpoint penting ESP32:

```http
GET  /api/sensor
GET  /api/calibration
POST /api/calibration
GET  /api/network
POST /api/wifi
```

Endpoint penting dashboard:

```http
GET  /api/cafes
POST /api/cafes
GET  /api/history
POST /api/history
POST /api/evaluate
PUT  /api/calibration
```

## Hardware

- ESP32 DOIT DevKit V1
- Sensor TDS analog, contoh Gravity Analog TDS Meter
- Sensor pH analog, contoh modul pH probe E-201-C
- Sensor suhu DS18B20 waterproof
- Resistor 4.7k ohm untuk DS18B20
- Kabel jumper
- Kabel USB data untuk ESP32
- Laptop/PC dengan VS Code dan PlatformIO

Catatan listrik:

- Pin ADC ESP32 hanya aman sampai 3.3V.
- Jika modul sensor diberi 5V, pastikan output analog ke ESP32 tidak lebih dari 3.3V.
- Semua GND wajib disambungkan jadi satu.

## Wiring ESP32

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

Tambahkan resistor 4.7k ohm:

```text
GPIO4 / DATA DS18B20 -> resistor 4.7k -> 3V3
```

Skema ringkas:

```text
ESP32 3V3  -> VCC TDS, VCC pH, VCC DS18B20
ESP32 GND  -> GND TDS, GND pH, GND DS18B20
ESP32 34   -> Analog Out TDS
ESP32 32   -> Analog Out pH
ESP32 4    -> DATA DS18B20
```

## Install Software

Install:

- Visual Studio Code
- Extension PlatformIO IDE
- Node.js LTS
- Git, jika belum ada

Buka folder project:

```text
C:\Users\Entung\Documents\PlatformIO\Projects\espresso
```

## Firmware ESP32

File firmware utama:

```text
src/main.cpp
```

Default WiFi ada di bagian:

```cpp
const char *DEFAULT_WIFI_SSID = "";
const char *DEFAULT_WIFI_PASSWORD = "";
```

Jika kosong, ESP32 membuat Access Point sendiri:

```text
SSID     : Espresso-Calibrator
Password : espresso123
IP       : 192.168.4.1
```

Firmware juga menyimpan WiFi ke Preferences. Jadi setelah upload, WiFi bisa diganti tanpa rebuild lewat endpoint:

```http
POST /api/wifi
```

## Build dan Upload Firmware

Dari root project:

```powershell
pio run
pio run --target upload
```

Jika `pio` tidak ditemukan di Windows, pakai path PlatformIO:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run --target upload
```

Jika upload gagal:

- Pastikan kabel USB mendukung data.
- Tutup Serial Monitor sebelum upload.
- Tekan dan tahan tombol `BOOT` saat upload mulai.
- Lepas tombol `BOOT` saat muncul proses `Writing at...`.
- Tekan `EN` atau `RST` setelah upload selesai.

## Serial Monitor

Buka monitor:

```powershell
pio device monitor
```

Atau:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" device monitor
```

Baud rate:

```text
115200
```

Output normal kira-kira:

```text
ESPRESSO CALIBRATION SYSTEM
REST API: /api/sensor /api/calibration
Dashboard device URL: http://192.168.x.x
Temperature : 92.00 C
TDS         : 8.90 % / 890 ppm
PH          : 5.24
Status      : Ideal Espresso
```

Jika belum join WiFi router:

```text
AP SSID     : Espresso-Calibrator
AP IP       : 192.168.4.1
Connect to AP, then open: http://192.168.4.1
```

## Tes ESP32 API

Mode AP ESP32:

```text
http://192.168.4.1/api/sensor
```

Mode ESP32 join WiFi router/hotspot:

```text
http://IP_ESP32/api/sensor
```

Contoh response:

```json
{
  "temperature": 92.0,
  "ph": 5.24,
  "tds": 8.9,
  "tdsPpm": 890,
  "status": "Ideal Espresso",
  "raw": {
    "tdsAdc": 1234,
    "phAdc": 3102,
    "phVoltage": 2.5
  },
  "calibration": {
    "tdsMin": 8.5,
    "tdsMax": 9.5,
    "phMin": 5.1,
    "phMax": 5.4,
    "tempMin": 88,
    "tempMax": 96,
    "tolerance": 0.2
  }
}
```

Tes network ESP32:

```text
http://192.168.4.1/api/network
```

## Setup Dashboard Lokal

Masuk folder dashboard:

```powershell
cd C:\Users\Entung\Documents\PlatformIO\Projects\espresso\dashboard
```

Install dependency:

```powershell
npm install
```

Buat file env lokal:

```text
dashboard/.env.local
```

Contoh isi untuk mode AP ESP32:

```env
DATABASE_URL="postgresql://USER:PASSWORD@HOST.neon.tech/DATABASE?sslmode=require"
NEXT_PUBLIC_DEFAULT_DEVICE_URL="http://192.168.4.1"
```

Contoh isi untuk ESP32 yang join WiFi/hotspot:

```env
DATABASE_URL="postgresql://USER:PASSWORD@HOST.neon.tech/DATABASE?sslmode=require"
NEXT_PUBLIC_DEFAULT_DEVICE_URL="http://192.168.1.25"
```

Jalankan:

```powershell
npm run dev
```

Buka:

```text
http://localhost:3000
```

Penting:

- `.env.local` harus ada di folder `dashboard`, bukan root project.
- Setelah mengubah `.env.local`, restart `npm run dev`.
- Jangan commit `.env.local` karena berisi credential database.

## Setup Neon Database

Di Neon:

1. Buat project PostgreSQL.
2. Copy connection string.
3. Masukkan ke `dashboard/.env.local` sebagai `DATABASE_URL`.
4. Pastikan connection string memakai `sslmode=require`.

Buat tabel dengan Prisma:

```powershell
cd C:\Users\Entung\Documents\PlatformIO\Projects\espresso\dashboard
npm run prisma:deploy
```

Alternatif development:

```powershell
npm run prisma:push
```

Project juga punya SQL siap paste untuk Neon SQL Editor:

```text
dashboard/prisma/seed-neon.sql
```

Cara pakai SQL seed:

1. Buka Neon dashboard.
2. Buka SQL Editor.
3. Paste isi `dashboard/prisma/seed-neon.sql`.
4. Run.

Seed membuat:

- Tabel `Cafe`
- Tabel `CalibrationSetting`
- Tabel `Measurement`
- Cafe default
- Calibration default
- Contoh history measurement

## Skenario Jaringan

Bagian ini penting karena ESP32 AP tidak punya internet. Pilih skenario sesuai kebutuhan.

### Skenario A: Kalibrasi Offline Pakai AP ESP32

Kondisi:

```text
Laptop -> WiFi Espresso-Calibrator -> ESP32
Laptop tidak punya internet
```

Yang bisa:

- Dashboard lokal bisa dibuka di `http://localhost:3000`.
- Dashboard bisa baca ESP32 di `http://192.168.4.1`.
- Dashboard bisa save calibration ke ESP32.

Yang tidak bisa:

- Save cafe/history ke Neon gagal karena laptop tidak punya internet.

Gunakan skenario ini untuk kalibrasi cepat tanpa database online.

### Skenario B: ESP32 dan Laptop di WiFi yang Sama, Ada Internet

Kondisi:

```text
Laptop -> WiFi/hotspot internet
ESP32  -> WiFi/hotspot yang sama
Dashboard lokal -> ESP32 + Neon
```

Yang bisa:

- Dashboard baca ESP32.
- Dashboard simpan cafe/history/calibration ke Neon.
- Dashboard save calibration ke ESP32.

Ini skenario terbaik untuk development lokal.

Cara membuat ESP32 join WiFi tanpa rebuild:

1. Connect laptop ke AP ESP32 dulu.
2. Jalankan command ini:

```powershell
Invoke-RestMethod `
  -Uri "http://192.168.4.1/api/wifi" `
  -Method Post `
  -ContentType "application/json" `
  -Body '{"ssid":"NAMA_WIFI","password":"PASSWORD_WIFI"}'
```

3. Pindahkan laptop ke WiFi/hotspot yang sama.
4. Lihat Serial Monitor ESP32.
5. Ambil IP baru, contoh:

```text
Dashboard device URL: http://192.168.1.25
```

6. Masukkan URL itu di dashboard.

Catatan hotspot:

- ESP32 hanya bisa 2.4 GHz.
- Gunakan WPA2.
- Hindari 5 GHz atau WPA3-only.

### Skenario C: Dashboard Vercel

Kondisi:

```text
Browser -> Vercel HTTPS
ESP32 lokal -> HTTP IP lokal
```

Masalah:

- Browser modern memblokir HTTPS page yang fetch ke HTTP ESP32 lokal. Ini disebut mixed content.
- Server Vercel juga tidak bisa akses IP lokal rumah/kafe kamu.

Yang bisa:

- Vercel bisa akses Neon.
- Dashboard Vercel bisa menampilkan data database.

Yang tidak bisa langsung:

- Vercel dashboard membaca `http://192.168.x.x/api/sensor` dari ESP32 lokal.

Solusi:

- Jalankan dashboard lokal saat kalibrasi.
- Atau pakai tunnel HTTPS ke ESP32/gateway lokal.
- Atau ubah arsitektur: ESP32 push measurement ke endpoint internet.

### Skenario D: Ingin Offline Tapi Tetap Simpan History

Neon tidak cocok jika laptop tidak punya internet. Pilihan alternatif:

- Simpan sementara di browser/localStorage lalu sync saat internet ada.
- Pakai database lokal di laptop.
- Gunakan hotspot HP yang punya internet dan sambungkan ESP32 ke hotspot itu.

## Cara Pakai Harian

1. Nyalakan ESP32.
2. Buka Serial Monitor dan lihat IP device.
3. Jalankan dashboard lokal:

```powershell
cd C:\Users\Entung\Documents\PlatformIO\Projects\espresso\dashboard
npm run dev
```

4. Buka `http://localhost:3000`.
5. Isi ESP32 URL sesuai Serial Monitor.
6. Pastikan status menjadi `Connected`.
7. Pilih cafe aktif atau buat cafe profile baru.
8. Lakukan beberapa percobaan espresso.
9. Jika satu shot sudah dirasa tepat oleh barista, klik `Set Standard`.
10. Dashboard menyimpan nilai TDS, pH, dan suhu shot tersebut sebagai standar cafe.
11. Shot berikutnya dievaluasi berdasarkan standar cafe itu.
12. Simpan measurement ke history jika database aktif.

## Workflow Standar Cafe

Fitur `Set Standard` dipakai saat barista sudah menemukan rasa espresso yang tepat. Contohnya, shot pertama dan kedua belum sesuai, lalu shot ketiga terasa pas. Barista menekan `Set Standard`, kemudian dashboard mengambil reading realtime terakhir dan menyimpannya sebagai standar cafe aktif.

Nilai standar dibuat dari reading terakhir dengan toleransi yang sedang dipakai:

```text
tdsMin  = TDS shot - tolerance
tdsMax  = TDS shot + tolerance
phMin   = pH shot - tolerance
phMax   = pH shot + tolerance
tempMin = suhu shot - tolerance
tempMax = suhu shot + tolerance
```

Setelah standar tersimpan, shot keempat dan seterusnya dibandingkan dengan standar tersebut agar rasa espresso lebih konsisten untuk cafe yang sama.

Catatan penting:

- `Set Standard` hanya aktif jika dashboard sudah menerima reading sensor.
- Cafe harus tersimpan di database, bukan `Default Cafe` lokal.
- `DATABASE_URL` harus aktif karena standar cafe disimpan ke PostgreSQL/Neon.
- Tolerance bisa diatur di panel `Calibration` sebelum menekan `Set Standard`.

## Kalibrasi Sensor

### Kalibrasi pH

Gunakan buffer pH seperti 4.00, 6.86/7.00, dan 9.18.

Firmware memakai konstanta:

```cpp
const float PH_NEUTRAL_VOLTAGE = 2.50f;
const float PH_SLOPE = 0.18f;
```

Langkah:

1. Celupkan probe ke buffer pH 6.86 atau 7.00.
2. Buka `/api/sensor`.
3. Lihat `raw.phVoltage`.
4. Pakai nilai itu sebagai acuan `PH_NEUTRAL_VOLTAGE`.
5. Cek buffer pH 4 dan 9.
6. Sesuaikan `PH_SLOPE` sampai pembacaan mendekati buffer.

### Kalibrasi TDS

Gunakan larutan standar TDS, misalnya 342 ppm, 707 ppm, atau 1000 ppm.

Langkah:

1. Pastikan output sensor TDS tidak lebih dari 3.3V.
2. Celupkan probe ke larutan standar.
3. Buka `/api/sensor`.
4. Bandingkan `tdsPpm` dengan larutan standar.
5. Jika perlu, tambahkan faktor koreksi di rumus `tdsPpm` pada `src/main.cpp`.

## Troubleshooting

### Error: Prisma `Can't reach database server`

Contoh error:

```text
prisma:error Invalid `prisma.cafe.create()` invocation:
Can't reach database server at `ep-xxxx-pooler...neon.tech:5432`
POST /api/cafes 400
```

Artinya dashboard tidak bisa menghubungi Neon.

Penyebab umum:

- Laptop sedang connect ke WiFi ESP32 `Espresso-Calibrator`, jadi tidak ada internet.
- Internet laptop putus.
- Neon sedang sleep/branch belum aktif.
- `DATABASE_URL` salah atau sudah diganti.
- Firewall/jaringan memblokir port PostgreSQL 5432.

Solusi cepat:

1. Pastikan laptop punya internet.
2. Jangan pakai WiFi ESP32 jika ingin save ke Neon.
3. Connect laptop ke WiFi/hotspot internet.
4. Connect ESP32 ke WiFi/hotspot yang sama.
5. Restart dashboard:

```powershell
Ctrl+C
npm run dev
```

6. Coba buat cafe lagi.

Tes koneksi Neon dari terminal dashboard:

```powershell
npm run prisma:deploy
```

Jika command itu juga gagal `Can't reach database server`, masalahnya koneksi internet/database, bukan dashboard UI.

### Error: `DATABASE_URL is not configured`

Artinya env belum terbaca.

Cek:

- File harus bernama `dashboard/.env.local`.
- Bukan `.env`, bukan `.env.example`, dan bukan di root project.
- Isi harus ada `DATABASE_URL=...`.
- Restart `npm run dev` setelah edit env.

Contoh benar:

```env
DATABASE_URL="postgresql://USER:PASSWORD@HOST.neon.tech/DATABASE?sslmode=require"
NEXT_PUBLIC_DEFAULT_DEVICE_URL="http://192.168.4.1"
```

### Error: Dashboard sensor `Failed to fetch`

Penyebab umum:

- ESP32 tidak satu jaringan dengan laptop.
- ESP32 URL salah.
- ESP32 belum menyala.
- Laptop masih di WiFi lain.
- Browser HTTPS mencoba fetch ESP32 HTTP.

Solusi:

1. Buka langsung endpoint ESP32 di browser:

```text
http://IP_ESP32/api/sensor
```

2. Kalau endpoint tidak terbuka, dashboard juga tidak akan bisa.
3. Cek IP di Serial Monitor.
4. Pastikan URL dashboard sama dengan IP Serial Monitor.

### Error: HTTPS dashboard tidak bisa fetch HTTP ESP32

Pesan dashboard:

```text
HTTPS dashboard cannot fetch an HTTP ESP URL
```

Artinya browser memblokir mixed content.

Solusi:

- Jalankan dashboard lokal dengan `npm run dev`.
- Jangan gunakan Vercel untuk membaca IP lokal ESP32 langsung.
- Jika wajib remote, gunakan HTTPS tunnel/gateway.

### Error: Save calibration ke ESP32 gagal

Cek:

- ESP32 URL di dashboard benar.
- Buka `http://IP_ESP32/api/calibration` di browser.
- Pastikan dashboard tidak dibuka dari HTTPS jika ESP32 masih HTTP.
- Pastikan range calibration valid: min harus lebih kecil dari max, tolerance lebih dari 0.

### ESP32 tidak bisa join hotspot

Cek:

- Hotspot harus 2.4 GHz.
- Security gunakan WPA2.
- Hindari WPA3-only.
- SSID/password case-sensitive.
- Jarak ESP32 ke hotspot jangan terlalu jauh.

Cek status network:

```text
http://192.168.4.1/api/network
```

Jika sudah punya IP baru, pakai IP baru itu di dashboard.

### API ESP32 tidak bisa dibuka

Cek:

- ESP32 menyala.
- Laptop satu jaringan dengan ESP32.
- Mode AP: laptop connect ke `Espresso-Calibrator`.
- Mode router/hotspot: laptop dan ESP32 join jaringan yang sama.
- Endpoint lengkap harus pakai `http://`, bukan `https://`.

### Nilai sensor aneh

Cek:

- Semua GND tersambung.
- Pin analog tidak menggantung.
- Output sensor analog tidak lebih dari 3.3V.
- Probe pH dan TDS sudah dikalibrasi.
- Kabel sensor jauh dari noise listrik.
- Power supply ESP32 stabil.

### `pio` tidak dikenali

Gunakan full path:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run
```

Atau tambahkan folder ini ke PATH:

```text
C:\Users\Entung\.platformio\penv\Scripts
```

## File Penting

```text
platformio.ini                         Konfigurasi board dan library ESP32
src/main.cpp                           Firmware ESP32
dashboard/                             Dashboard Next.js
dashboard/.env.local                   Env lokal, jangan commit
dashboard/.env.example                 Contoh env
dashboard/prisma/schema.prisma         Schema Prisma
dashboard/prisma/seed-neon.sql         SQL setup + seed untuk Neon
dashboard/components/dashboard.tsx     UI dashboard utama
dashboard/app/api/*                    API backend dashboard
```

## Checklist Jika Mau Demo

Sebelum demo:

- Firmware berhasil di-upload.
- Serial Monitor menampilkan IP ESP32.
- `http://IP_ESP32/api/sensor` bisa dibuka.
- Dashboard lokal bisa dibuka.
- `.env.local` sudah berisi `DATABASE_URL`.
- Laptop punya internet jika ingin save ke Neon.
- ESP32 dan laptop berada di jaringan yang sama jika ingin realtime sensor.
- Jangan pakai Vercel untuk membaca ESP32 lokal secara langsung.

## Command Cepat

Dashboard lokal:

```powershell
cd C:\Users\Entung\Documents\PlatformIO\Projects\espresso\dashboard
npm run dev
```

Build dashboard:

```powershell
npm run build
```

Build firmware:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run
```

Upload firmware:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run --target upload
```

Set WiFi ESP32:

```powershell
Invoke-RestMethod `
  -Uri "http://192.168.4.1/api/wifi" `
  -Method Post `
  -ContentType "application/json" `
  -Body '{"ssid":"NAMA_WIFI","password":"PASSWORD_WIFI"}'
```
