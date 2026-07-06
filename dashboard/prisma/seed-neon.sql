-- Neon/PostgreSQL setup + seed data for espresso-dashboard.
-- Safe to run more than once in Neon SQL Editor.

BEGIN;

CREATE TABLE IF NOT EXISTS "Cafe" (
    "id" TEXT NOT NULL,
    "name" TEXT NOT NULL,
    "location" TEXT,
    "createdAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "updatedAt" TIMESTAMP(3) NOT NULL,
    CONSTRAINT "Cafe_pkey" PRIMARY KEY ("id")
);

CREATE TABLE IF NOT EXISTS "CalibrationSetting" (
    "id" TEXT NOT NULL,
    "cafeId" TEXT NOT NULL,
    "tdsMin" DOUBLE PRECISION NOT NULL,
    "tdsMax" DOUBLE PRECISION NOT NULL,
    "phMin" DOUBLE PRECISION NOT NULL,
    "phMax" DOUBLE PRECISION NOT NULL,
    "tempMin" DOUBLE PRECISION NOT NULL,
    "tempMax" DOUBLE PRECISION NOT NULL,
    "tolerance" DOUBLE PRECISION NOT NULL,
    "createdAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "updatedAt" TIMESTAMP(3) NOT NULL,
    CONSTRAINT "CalibrationSetting_pkey" PRIMARY KEY ("id")
);

CREATE TABLE IF NOT EXISTS "Measurement" (
    "id" TEXT NOT NULL,
    "cafeId" TEXT,
    "temperature" DOUBLE PRECISION NOT NULL,
    "ph" DOUBLE PRECISION NOT NULL,
    "tds" DOUBLE PRECISION NOT NULL,
    "tdsPpm" DOUBLE PRECISION,
    "status" TEXT NOT NULL,
    "createdAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT "Measurement_pkey" PRIMARY KEY ("id")
);

CREATE UNIQUE INDEX IF NOT EXISTS "CalibrationSetting_cafeId_key"
ON "CalibrationSetting"("cafeId");

CREATE INDEX IF NOT EXISTS "Measurement_createdAt_idx"
ON "Measurement"("createdAt");

CREATE INDEX IF NOT EXISTS "Measurement_cafeId_idx"
ON "Measurement"("cafeId");

DO $$
BEGIN
    IF NOT EXISTS (
        SELECT 1 FROM pg_constraint WHERE conname = 'CalibrationSetting_cafeId_fkey'
    ) THEN
        ALTER TABLE "CalibrationSetting"
        ADD CONSTRAINT "CalibrationSetting_cafeId_fkey"
        FOREIGN KEY ("cafeId") REFERENCES "Cafe"("id")
        ON DELETE CASCADE ON UPDATE CASCADE;
    END IF;

    IF NOT EXISTS (
        SELECT 1 FROM pg_constraint WHERE conname = 'Measurement_cafeId_fkey'
    ) THEN
        ALTER TABLE "Measurement"
        ADD CONSTRAINT "Measurement_cafeId_fkey"
        FOREIGN KEY ("cafeId") REFERENCES "Cafe"("id")
        ON DELETE SET NULL ON UPDATE CASCADE;
    END IF;
END $$;

INSERT INTO "Cafe" ("id", "name", "location", "createdAt", "updatedAt")
VALUES
    ('cafe_default', 'Default Cafe', 'Local ESP32 calibration', CURRENT_TIMESTAMP, CURRENT_TIMESTAMP),
    ('cafe_lab', 'Lab Espresso', 'QC Lab', CURRENT_TIMESTAMP, CURRENT_TIMESTAMP)
ON CONFLICT ("id") DO UPDATE SET
    "name" = EXCLUDED."name",
    "location" = EXCLUDED."location",
    "updatedAt" = CURRENT_TIMESTAMP;

INSERT INTO "CalibrationSetting" (
    "id",
    "cafeId",
    "tdsMin",
    "tdsMax",
    "phMin",
    "phMax",
    "tempMin",
    "tempMax",
    "tolerance",
    "createdAt",
    "updatedAt"
)
VALUES
    ('cal_default', 'cafe_default', 8.5, 9.5, 5.1, 5.4, 88.0, 96.0, 0.2, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP),
    ('cal_lab', 'cafe_lab', 8.7, 9.3, 5.15, 5.35, 90.0, 94.0, 0.2, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP)
ON CONFLICT ("cafeId") DO UPDATE SET
    "tdsMin" = EXCLUDED."tdsMin",
    "tdsMax" = EXCLUDED."tdsMax",
    "phMin" = EXCLUDED."phMin",
    "phMax" = EXCLUDED."phMax",
    "tempMin" = EXCLUDED."tempMin",
    "tempMax" = EXCLUDED."tempMax",
    "tolerance" = EXCLUDED."tolerance",
    "updatedAt" = CURRENT_TIMESTAMP;

INSERT INTO "Measurement" (
    "id",
    "cafeId",
    "temperature",
    "ph",
    "tds",
    "tdsPpm",
    "status",
    "createdAt"
)
VALUES
    ('measure_seed_001', 'cafe_default', 92.0, 5.24, 8.9, 890, 'Ideal Espresso', CURRENT_TIMESTAMP - INTERVAL '15 minutes'),
    ('measure_seed_002', 'cafe_default', 86.8, 5.02, 8.1, 810, 'Under Extract', CURRENT_TIMESTAMP - INTERVAL '10 minutes'),
    ('measure_seed_003', 'cafe_default', 97.2, 5.52, 9.9, 990, 'Over Extract', CURRENT_TIMESTAMP - INTERVAL '5 minutes')
ON CONFLICT ("id") DO NOTHING;

COMMIT;