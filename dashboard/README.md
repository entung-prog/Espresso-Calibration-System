# Espresso Dashboard

Next.js dashboard untuk monitoring ESP32 Coffee Calibration System.

## Fitur

- Realtime card untuk TDS, pH, temperature, dan status ekstraksi
- Fuzzy Mamdani berjalan di backend endpoint `/api/evaluate`
- Grafik realtime memakai Chart.js
- Form kalibrasi yang disimpan di backend/PostgreSQL
- Cafe profile
- History measurement ke PostgreSQL
- Export history CSV

## Development

```powershell
npm install
npm run dev
```

Buka `http://localhost:3000`.

## Environment

Buat `.env.local` saat development:

```env
DATABASE_URL="postgresql://USER:PASSWORD@HOST:5432/DATABASE?sslmode=require"
NEXT_PUBLIC_DEFAULT_DEVICE_URL="http://192.168.4.1"
```

Kalau dashboard dipakai di Vercel, kosongkan `NEXT_PUBLIC_DEFAULT_DEVICE_URL` atau pakai URL HTTPS publik/tunnel. Browser HTTPS tidak bisa fetch ke IP lokal ESP32 langsung.

`DATABASE_URL` dipakai untuk cafe, calibration, dan history. Tanpa database, dashboard tetap terbuka, tetapi fitur simpan data akan memberi pesan bahwa database belum aktif.
## Database

Untuk membuat tabel PostgreSQL:

```powershell
npm run prisma:deploy
```

Alternatif saat development:

```powershell
npm run prisma:push
```

Provider PostgreSQL yang cocok untuk Vercel: Vercel Postgres, Neon, Supabase, atau Railway.

## Deploy Vercel

Set project root Vercel ke folder `dashboard`.

Build command:

```text
npm run build
```

Install command:

```text
npm install
```

Environment variables di Vercel:

```text
DATABASE_URL
NEXT_PUBLIC_DEFAULT_DEVICE_URL (opsional, hanya untuk URL device HTTPS publik/tunnel)
```

Setelah deploy pertama, jalankan migration dari lokal dengan `DATABASE_URL` production:

```powershell
npm run prisma:deploy
```

## Logic Fuzzy

Logic Fuzzy Mamdani ada di:

```text
lib/fuzzy.ts
```

Dashboard mengambil data mentah dari ESP32 `/api/sensor`, lalu mengirimnya ke:

```http
POST /api/evaluate
```

Endpoint backend ini memakai calibration profile dari PostgreSQL jika `cafeId`
tersedia. Jika database belum aktif, endpoint memakai default calibration dari
`lib/validation.ts`.

## Catatan ESP32 dan Vercel

Dashboard Vercel berjalan via HTTPS. Browser modern sering memblokir fetch dari HTTPS ke `http://IP_ESP32` karena mixed content. Server Vercel juga tidak bisa mengakses IP lokal ESP32.

Hotspot HP bisa dipakai, tapi harus 2.4 GHz dan WPA2. ESP32 tidak bisa join 5 GHz atau WPA3.

Pilihan produksi yang lebih stabil:

- Jalankan dashboard lokal saat kalibrasi di jaringan yang sama dengan ESP32
- Buat ESP32 mengirim measurement ke endpoint `/api/history` lewat internet
- Pakai gateway lokal, MQTT broker, atau tunnel HTTPS untuk menjembatani ESP32
