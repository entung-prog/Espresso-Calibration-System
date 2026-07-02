import { NextResponse } from "next/server";
import { databaseMissingResponse, errorResponse, hasDatabaseUrl } from "@/lib/api";
import { prisma } from "@/lib/prisma";
import { defaultCalibration } from "@/lib/validation";

export async function GET() {
  if (!hasDatabaseUrl()) {
    return databaseMissingResponse();
  }

  const cafes = await prisma.cafe.findMany({
    include: { calibration: true },
    orderBy: { createdAt: "asc" },
  });

  return NextResponse.json({ items: cafes });
}

export async function POST(request: Request) {
  if (!hasDatabaseUrl()) {
    return databaseMissingResponse();
  }

  try {
    const body = await request.json();
    const name = String(body.name ?? "").trim();
    const location = String(body.location ?? "").trim();

    if (!name) {
      throw new Error("Cafe name is required");
    }

    const cafe = await prisma.cafe.create({
      data: {
        name,
        location: location || null,
        calibration: {
          create: defaultCalibration,
        },
      },
      include: { calibration: true },
    });

    return NextResponse.json(cafe, { status: 201 });
  } catch (error) {
    return errorResponse(error);
  }
}
