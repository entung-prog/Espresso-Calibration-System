CREATE TABLE "Cafe" (
    "id" TEXT NOT NULL,
    "name" TEXT NOT NULL,
    "location" TEXT,
    "createdAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "updatedAt" TIMESTAMP(3) NOT NULL,

    CONSTRAINT "Cafe_pkey" PRIMARY KEY ("id")
);

CREATE TABLE "CalibrationSetting" (
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

CREATE TABLE "Measurement" (
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

CREATE UNIQUE INDEX "CalibrationSetting_cafeId_key" ON "CalibrationSetting"("cafeId");
CREATE INDEX "Measurement_createdAt_idx" ON "Measurement"("createdAt");
CREATE INDEX "Measurement_cafeId_idx" ON "Measurement"("cafeId");

ALTER TABLE "CalibrationSetting"
ADD CONSTRAINT "CalibrationSetting_cafeId_fkey"
FOREIGN KEY ("cafeId") REFERENCES "Cafe"("id") ON DELETE CASCADE ON UPDATE CASCADE;

ALTER TABLE "Measurement"
ADD CONSTRAINT "Measurement_cafeId_fkey"
FOREIGN KEY ("cafeId") REFERENCES "Cafe"("id") ON DELETE SET NULL ON UPDATE CASCADE;
