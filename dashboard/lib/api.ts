import { NextResponse } from "next/server";

export function databaseMissingResponse() {
  return NextResponse.json(
    {
      error: "DATABASE_URL is not configured",
      hint: "Set DATABASE_URL in Vercel or .env.local to enable PostgreSQL storage.",
    },
    { status: 503 },
  );
}

export function hasDatabaseUrl() {
  return Boolean(process.env.DATABASE_URL);
}

export function errorResponse(error: unknown, status = 400) {
  const message = error instanceof Error ? error.message : "Request failed";
  return NextResponse.json({ error: message }, { status });
}
