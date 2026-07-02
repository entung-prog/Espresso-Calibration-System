import { NextResponse } from "next/server";
import { errorResponse, hasDatabaseUrl } from "@/lib/api";
import { classifyExtraction } from "@/lib/fuzzy";
import { prisma } from "@/lib/prisma";
import { defaultCalibration, parseCalibration, parseReading } from "@/lib/validation";

export async function POST(request: Request) {
  try {
    const body = await request.json();
    const reading = parseReading(body);
    const cafeId =
      typeof body.cafeId === "string" && body.cafeId.trim()
        ? body.cafeId.trim()
        : null;

    let calibration = body.calibration
      ? parseCalibration(body.calibration)
      : defaultCalibration;

    if (cafeId && hasDatabaseUrl()) {
      const savedCalibration = await prisma.calibrationSetting.findUnique({
        where: { cafeId },
      });

      if (savedCalibration) {
        calibration = {
          tdsMin: savedCalibration.tdsMin,
          tdsMax: savedCalibration.tdsMax,
          phMin: savedCalibration.phMin,
          phMax: savedCalibration.phMax,
          tempMin: savedCalibration.tempMin,
          tempMax: savedCalibration.tempMax,
          tolerance: savedCalibration.tolerance,
        };
      }
    }

    const result = classifyExtraction(reading, calibration);

    return NextResponse.json({
      ...reading,
      status: result.status,
      fuzzy: {
        score: result.score,
        membership: result.membership,
      },
      calibration,
    });
  } catch (error) {
    return errorResponse(error);
  }
}
