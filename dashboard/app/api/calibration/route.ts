import { NextResponse } from "next/server";
import { databaseMissingResponse, errorResponse, hasDatabaseUrl } from "@/lib/api";
import { prisma } from "@/lib/prisma";
import { assertCalibration, parseCalibration } from "@/lib/validation";

export async function GET(request: Request) {
  if (!hasDatabaseUrl()) {
    return databaseMissingResponse();
  }

  const { searchParams } = new URL(request.url);
  const cafeId = searchParams.get("cafeId");

  if (!cafeId) {
    return errorResponse(new Error("cafeId is required"));
  }

  const calibration = await prisma.calibrationSetting.findUnique({
    where: { cafeId },
  });

  return NextResponse.json(calibration);
}

export async function PUT(request: Request) {
  if (!hasDatabaseUrl()) {
    return databaseMissingResponse();
  }

  try {
    const body = await request.json();
    const cafeId = String(body.cafeId ?? "").trim();

    if (!cafeId) {
      throw new Error("cafeId is required");
    }

    const calibration = parseCalibration(body);
    assertCalibration(calibration);

    const saved = await prisma.calibrationSetting.upsert({
      where: { cafeId },
      update: calibration,
      create: {
        cafeId,
        ...calibration,
      },
    });

    return NextResponse.json(saved);
  } catch (error) {
    return errorResponse(error);
  }
}

export const POST = PUT;
