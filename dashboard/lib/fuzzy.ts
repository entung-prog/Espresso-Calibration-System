import type { Calibration, SensorReading } from "@/lib/types";
import { defaultCalibration } from "@/lib/validation";

function clamp01(value: number) {
  return Math.min(1, Math.max(0, value));
}

function rising(value: number, start: number, end: number) {
  if (value <= start) {
    return 0;
  }

  if (value >= end) {
    return 1;
  }

  return clamp01((value - start) / (end - start));
}

function falling(value: number, start: number, end: number) {
  if (value <= start) {
    return 1;
  }

  if (value >= end) {
    return 0;
  }

  return clamp01((end - value) / (end - start));
}

function trapezoid(value: number, a: number, b: number, c: number, d: number) {
  return Math.min(rising(value, a, b), falling(value, c, d));
}

export function classifyExtraction(
  reading: Pick<SensorReading, "tds" | "ph" | "temperature">,
  calibration: Calibration = defaultCalibration,
) {
  const tdsTolerance = calibration.tolerance;
  const phTolerance = calibration.tolerance;

  const tdsLow = falling(reading.tds, calibration.tdsMin - tdsTolerance, calibration.tdsMin);
  const tdsIdeal = trapezoid(
    reading.tds,
    calibration.tdsMin - tdsTolerance,
    calibration.tdsMin,
    calibration.tdsMax,
    calibration.tdsMax + tdsTolerance,
  );
  const tdsHigh = rising(reading.tds, calibration.tdsMax, calibration.tdsMax + tdsTolerance);

  const phAcid = falling(reading.ph, calibration.phMin - phTolerance, calibration.phMin);
  const phIdeal = trapezoid(
    reading.ph,
    calibration.phMin - phTolerance,
    calibration.phMin,
    calibration.phMax,
    calibration.phMax + phTolerance,
  );
  const phBase = rising(reading.ph, calibration.phMax, calibration.phMax + phTolerance);

  // Temperature is retained for TDS compensation and display, but extraction
  // status is determined by the primary TDS and pH measurements.
  const underExtract = Math.max(tdsLow, phAcid);
  const idealEspresso = Math.min(tdsIdeal, phIdeal);
  const overExtract = Math.max(tdsHigh, phBase);
  const totalWeight = underExtract + idealEspresso + overExtract;

  if (totalWeight <= 0) {
    return {
      status: "Unknown",
      score: 0,
      membership: { underExtract, idealEspresso, overExtract },
    };
  }

  const score = (-1 * underExtract + overExtract) / totalWeight;
  const status =
    score < -0.25 ? "Under Extract" : score > 0.25 ? "Over Extract" : "Ideal Espresso";

  return {
    status,
    score,
    membership: { underExtract, idealEspresso, overExtract },
  };
}
