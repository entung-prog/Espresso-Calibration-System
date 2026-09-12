# Espresso Dashboard

Dashboard Next.js untuk monitoring ESP32 Coffee Calibration System.

Dashboard membaca sensor ESP32 lewat HTTP, mengevaluasi status ekstraksi espresso, dan menyimpan cafe, standar kalibrasi, serta history measurement ke PostgreSQL jika `DATABASE_URL` aktif.

## Fitur

- Realtime card untuk TDS, pH, temperature, dan status ekstraksi.
- Grafik realtime memakai Chart.js.
- Cafe profile untuk menyimpan standar tiap cafe.
- Tombol `Set Standard` untuk menjadikan shot espresso terakhir sebagai standar cafe.
- Calibration profile manual.
- History measurement ke PostgreSQL/Neon.
- Export history ke CSV.
- Save calibration ke ESP32 lewat `/api/calibration`.
- Evaluation endpoint `/api/evaluate`.

## Workflow Barista

1. Jalankan dashboard lokal.
2. Hubungkan dashboard ke ESP32 lewat field `ESP32 URL`.
3. Pilih cafe aktif atau buat cafe baru.
4. Barista membuat beberapa shot espresso.
5. Saat shot terasa paling tepat, klik `Set Standard`.
6. Dashboard menyimpan nilai TDS dan pH shot itu sebagai standar cafe; temperature hanya dicatat sebagai informasi.
7. Shot berikutnya dievaluasi memakai standar cafe tersebut.
8. Jika perlu, simpan measurement ke history atau aktifkan mode `Auto`.

Contoh skenario:

```text
Shot 1  -> belum pas
Shot 2  -> belum pas
Shot 3  -> rasa pas, klik Set Standard
Shot 4+ -> dibandingkan dengan standar dari shot 3
```

## Cara Kerja Set Standard

`Set Standard` mengambil reading realtime terakhir dan membuat range kalibrasi memakai nilai tolerance yang sedang aktif.

```text
tdsMin  = TDS shot - tolerance
tdsMax  = TDS shot + tolerance
phMin   = pH shot - tolerance
phMax   = pH shot + tolerance
```

Syarat:

- Dashboard sudah menerima reading sensor terbaru.
- Cafe aktif bukan `Default Cafe` lokal.
- `DATABASE_URL` aktif karena standar disimpan ke database.
- Tolerance harus lebih dari 0.

Status ekstraksi memakai TDS dan pH sebagai parameter utama. Temperature tetap tersimpan dan tampil sebagai informasi pendukung (serta dipakai firmware untuk kompensasi TDS), sehingga perubahan suhu saja tidak mengubah status menjadi `Under Extract` atau `Over Extract`.

## Development Lokal

```powershell
cd C:\Users\Entung\Documents\PlatformIO\Projects\espresso\dashboard
npm install
npm run dev
```

Buka:

```text
http://localhost:3000
```

## Environment

Buat file:

```text
dashboard/.env.local
```

Contoh database lokal:

```env
DATABASE_URL="postgresql://espresso:espresso@localhost:5432/espresso?schema=public"
NEXT_PUBLIC_DEFAULT_DEVICE_URL="http://192.168.4.1"
```

Catatan:

- `.env.local` harus di folder `dashboard`.
- Restart `npm run dev` setelah mengubah `.env.local`.
- Jangan commit `.env.local`.
- `DATABASE_URL` dipakai untuk cafe, calibration, dan history.
- `NEXT_PUBLIC_DEFAULT_DEVICE_URL` dipakai sebagai default URL ESP32.

## Database Lokal

Cara paling sederhana untuk development lokal adalah PostgreSQL via Docker.

```powershell
cd C:\Users\Entung\Documents\PlatformIO\Projects\espresso\dashboard
npm run db:up
npm run prisma:migrate
npm run dev
```

Untuk melihat isi database:

```powershell
npm run db:studio
```

Database lokal memakai kredensial:

```text
Host     : localhost
Port     : 5432
Database : espresso
User     : espresso
Password : espresso
```

## Database Neon

Buat tabel dengan Prisma:

```powershell
npm run prisma:deploy
```

Alternatif development:

```powershell
npm run prisma:push
```

SQL setup + seed manual tersedia di:

```text
prisma/seed-neon.sql
```

Paste file itu ke Neon SQL Editor jika ingin setup manual.

## Endpoint Dashboard

```http
GET  /api/cafes
POST /api/cafes
GET  /api/calibration?cafeId=...
PUT  /api/calibration
POST /api/evaluate
GET  /api/history
POST /api/history
```

## Skenario Koneksi

### ESP32 Access Point

Laptop connect ke WiFi ESP32:

```text
SSID     : Espresso-Calibrator
Password : espresso123
ESP32    : http://192.168.4.1
```

Yang bisa:

- Dashboard lokal membaca sensor.
- Dashboard save calibration ke ESP32.

Yang tidak bisa:

- Save cafe, standar cafe, dan history ke Neon karena laptop biasanya tidak punya internet saat connect ke AP ESP32.

### WiFi/Hotspot Bersama

Laptop dan ESP32 join WiFi/hotspot yang sama dan ada internet.

Yang bisa:

- Dashboard membaca sensor ESP32.
- Dashboard menyimpan cafe, standar cafe, dan history ke Neon.
- Dashboard save calibration ke ESP32.

Ini mode terbaik untuk development.

### Vercel

Vercel berjalan via HTTPS. Browser akan memblokir fetch dari HTTPS dashboard ke `http://IP_ESP32` karena mixed content.

Untuk kalibrasi realtime, jalankan dashboard lokal. Untuk production remote, gunakan HTTPS tunnel/gateway atau ubah ESP32 agar push data ke server.

## Error Umum

### Prisma `Can't reach database server`

Artinya dashboard tidak bisa menghubungi Neon.

Penyebab paling umum:

- Laptop sedang connect ke WiFi ESP32 yang tidak punya internet.
- Internet laptop putus.
- `DATABASE_URL` salah.
- Neon branch/project sedang tidak bisa dijangkau.
- Jaringan memblokir PostgreSQL port 5432.

Solusi:

1. Connect laptop ke WiFi/hotspot yang ada internet.
2. Pastikan ESP32 juga join jaringan yang sama jika ingin realtime sensor.
3. Restart `npm run dev`.
4. Test ulang create cafe, `Set Standard`, atau save history.

### `DATABASE_URL is not configured`

Penyebab:

- File `.env.local` belum ada.
- File env ada di folder yang salah.
- Dev server belum direstart setelah env dibuat.

Solusi:

1. Buat `dashboard/.env.local`.
2. Isi `DATABASE_URL`.
3. Stop dev server.
4. Jalankan lagi `npm run dev`.

### HTTPS Dashboard Cannot Fetch HTTP ESP URL

Penyebab:

- Dashboard dibuka dari HTTPS, tapi ESP32 hanya punya HTTP lokal.

Solusi:

- Gunakan dashboard lokal `http://localhost:3000` saat kalibrasi.
- Jangan arahkan Vercel langsung ke IP lokal ESP32.
- Pakai HTTPS tunnel/gateway jika perlu remote access.

### Dashboard Tidak Membaca Sensor

Cek langsung di browser:

```text
http://IP_ESP32/api/sensor
```

Jika endpoint itu tidak terbuka, dashboard juga tidak bisa membaca sensor.

Cek:

- ESP32 menyala.
- Laptop dan ESP32 satu jaringan.
- URL memakai `http://`, bukan `https://`.
- IP sesuai Serial Monitor.

## Deploy Vercel

Set project root Vercel ke folder:

```text
dashboard
```

Build command:

```text
npm run build
```

Install command:

```text
npm install
```

Environment variables:

```text
DATABASE_URL
NEXT_PUBLIC_DEFAULT_DEVICE_URL
```

Untuk Vercel, `NEXT_PUBLIC_DEFAULT_DEVICE_URL` sebaiknya kosong kecuali kamu punya URL HTTPS publik/tunnel untuk device.

Setelah deploy pertama, jalankan migration dengan `DATABASE_URL` production:

```powershell
npm run prisma:deploy
```

## Logic Fuzzy

Logic fuzzy ada di:

```text
lib/fuzzy.ts
```

Validation default calibration ada di:

```text
lib/validation.ts
```

Dashboard mengirim reading ke:

```http
POST /api/evaluate
```

Jika database aktif dan `cafeId` tersedia, endpoint memakai calibration profile dari database. Jika database tidak aktif, endpoint memakai default calibration.
