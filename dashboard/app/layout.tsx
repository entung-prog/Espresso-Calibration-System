import type { Metadata } from "next";
import "./globals.css";

export const metadata: Metadata = {
  title: "Espresso Calibration Dashboard",
  description: "Realtime espresso TDS, pH, temperature, calibration, and history.",
};

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html lang="en">
      <body>{children}</body>
    </html>
  );
}
