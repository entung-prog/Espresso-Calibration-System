import { NextResponse } from "next/server";
import { databaseMissingResponse, errorResponse, hasDatabaseUrl } from "@/lib/api";
import { prisma } from "@/lib/prisma";
import { parseReading } from "@/lib/validation";

export async function GET(request: Request) {
  if (!hasDatabaseUrl()) {
    return databaseMissingResponse();
  }

  const { searchParams } = new URL(request.url);
  const cafeId = searchParams.get("cafeId");
  const take = Math.min(Number(searchParams.get("take") ?? 50), 200);

  const measurements = await prisma.measurement.findMany({
    where: cafeId ? { cafeId } : undefined,
    include: { cafe: true },
    orderBy: { createdAt: "desc" },
    take,
  });

  return NextResponse.json({ items: measurements });
}

export async function POST(request: Request) {
  if (!hasDatabaseUrl()) {
    return databaseMissingResponse();
  }

  try {
    const body = await request.json();
    const reading = parseReading(body);
    const cafeId =
      typeof body.cafeId === "string" && body.cafeId.trim()
        ? body.cafeId.trim()
        : null;

    const measurement = await prisma.measurement.create({
      data: {
        cafeId,
        temperature: reading.temperature,
        ph: reading.ph,
        tds: reading.tds,
        tdsPpm: reading.tdsPpm,
        status: reading.status,
      },
    });

    return NextResponse.json(measurement, { status: 201 });
  } catch (error) {
    return errorResponse(error);
  }
}
