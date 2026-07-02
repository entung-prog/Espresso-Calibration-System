import type { Calibration, SensorReading } from "@/lib/types";

export const defaultCalibration: Calibration = {
  tdsMin: 8.5,
  tdsMax: 9.5,
  phMin: 5.1,
  phMax: 5.4,
  tempMin: 88,
  tempMax: 96,
  tolerance: 0.2,
};

export function numberFrom(value: unknown, fallback: number) {
  const parsed = typeof value === "number" ? value : Number(value);
  return Number.isFinite(parsed) ? parsed : fallback;
}

export function parseCalibration(input: Record<string, unknown>): Calibration {
  return {
    tdsMin: numberFrom(input.tdsMin, defaultCalibration.tdsMin),
    tdsMax: numberFrom(input.tdsMax, defaultCalibration.tdsMax),
    phMin: numberFrom(input.phMin, defaultCalibration.phMin),
    phMax: numberFrom(input.phMax, defaultCalibration.phMax),
    tempMin: numberFrom(input.tempMin, defaultCalibration.tempMin),
    tempMax: numberFrom(input.tempMax, defaultCalibration.tempMax),
    tolerance: numberFrom(input.tolerance, defaultCalibration.tolerance),
  };
}

export function assertCalibration(calibration: Calibration) {
  if (
    calibration.tdsMin >= calibration.tdsMax ||
    calibration.phMin >= calibration.phMax ||
    calibration.tempMin >= calibration.tempMax ||
    calibration.tolerance <= 0
  ) {
    throw new Error("Invalid calibration range");
  }
}

export function parseReading(input: Record<string, unknown>): SensorReading {
  return {
    temperature: numberFrom(input.temperature, 0),
    ph: numberFrom(input.ph, 0),
    tds: numberFrom(input.tds, 0),
    tdsPpm:
      input.tdsPpm === undefined ? undefined : numberFrom(input.tdsPpm, 0),
    status: typeof input.status === "string" ? input.status : "Unknown",
  };
}
