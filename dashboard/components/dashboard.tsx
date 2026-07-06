"use client";

import {
  Activity,
  Building2,
  Database,
  Download,
  Gauge,
  Play,
  Save,
  Settings2,
  Square,
  Thermometer,
  Wifi,
} from "lucide-react";
import {
  CategoryScale,
  Chart as ChartJS,
  Legend,
  LinearScale,
  LineElement,
  PointElement,
  Tooltip,
} from "chart.js";
import { Line } from "react-chartjs-2";
import { FormEvent, useEffect, useMemo, useState } from "react";
import type { Cafe, Calibration, SensorReading } from "@/lib/types";
import { assertCalibration, defaultCalibration } from "@/lib/validation";

ChartJS.register(CategoryScale, LinearScale, PointElement, LineElement, Tooltip, Legend);

type HistoryItem = SensorReading & {
  id?: string;
  cafe?: { name: string } | null;
};

const fallbackCafe: Cafe = {
  id: "local",
  name: "Default Cafe",
  location: "Local device",
  calibration: defaultCalibration,
};

function statusClass(status: string) {
  if (status.toLowerCase().includes("ideal")) {
    return "bg-sage text-white";
  }

  if (status.toLowerCase().includes("over")) {
    return "bg-cherry text-white";
  }

  if (status.toLowerCase().includes("under")) {
    return "bg-crema text-ink";
  }

  return "bg-stone-200 text-stone-700";
}

function formatNumber(value: number | undefined, digits = 2) {
  if (value === undefined || Number.isNaN(value)) {
    return "--";
  }

  return value.toFixed(digits);
}

function normalizeDeviceUrl(url: string) {
  const trimmed = url.trim().replace(/\/$/, "");
  if (!trimmed) {
    return "";
  }

  if (/^https?:\/\//i.test(trimmed)) {
    return trimmed;
  }

  return `http://${trimmed}`;
}

export function Dashboard() {
  const [deviceUrl, setDeviceUrl] = useState("");
  const [draftDeviceUrl, setDraftDeviceUrl] = useState("");
  const [isPolling, setIsPolling] = useState(true);
  const [isSavingHistory, setIsSavingHistory] = useState(false);
  const [connectionStatus, setConnectionStatus] = useState("Not connected");
  const [latest, setLatest] = useState<SensorReading | null>(null);
  const [series, setSeries] = useState<SensorReading[]>([]);
  const [cafes, setCafes] = useState<Cafe[]>([fallbackCafe]);
  const [selectedCafeId, setSelectedCafeId] = useState(fallbackCafe.id);
  const [calibration, setCalibration] = useState<Calibration>(defaultCalibration);
  const [history, setHistory] = useState<HistoryItem[]>([]);
  const [newCafeName, setNewCafeName] = useState("");
  const [newCafeLocation, setNewCafeLocation] = useState("");
  const [notice, setNotice] = useState("");

  const selectedCafe = cafes.find((cafe) => cafe.id === selectedCafeId) ?? cafes[0];

  useEffect(() => {
    const savedUrl = localStorage.getItem("espressoDeviceUrl");
    const fallbackUrl =
      process.env.NEXT_PUBLIC_DEFAULT_DEVICE_URL ??
      (window.location.protocol === "https:" ? "" : "http://192.168.4.1");
    const nextUrl = savedUrl ?? fallbackUrl;
    setDeviceUrl(nextUrl);
    setDraftDeviceUrl(nextUrl);
  }, []);

  useEffect(() => {
    void loadCafes();
    void loadHistory();
  }, []);

  useEffect(() => {
    if (selectedCafe?.calibration) {
      setCalibration({
        tdsMin: selectedCafe.calibration.tdsMin,
        tdsMax: selectedCafe.calibration.tdsMax,
        phMin: selectedCafe.calibration.phMin,
        phMax: selectedCafe.calibration.phMax,
        tempMin: selectedCafe.calibration.tempMin,
        tempMax: selectedCafe.calibration.tempMax,
        tolerance: selectedCafe.calibration.tolerance,
      });
    }
  }, [selectedCafe]);

  useEffect(() => {
    if (!deviceUrl || !isPolling) {
      return;
    }

    let cancelled = false;
    const normalizedDeviceUrl = normalizeDeviceUrl(deviceUrl);

    let parsedUrl: URL;
    try {
      parsedUrl = new URL(normalizedDeviceUrl);
    } catch {
      setConnectionStatus("Device URL is invalid");
      return;
    }

    if (window.location.protocol === "https:" && parsedUrl.protocol === "http:") {
      setConnectionStatus(
        "HTTPS dashboard cannot fetch an HTTP ESP URL. Use a local dashboard or an HTTPS tunnel.",
      );
      return;
    }

    async function poll() {
      try {
        const response = await fetch(`${normalizedDeviceUrl}/api/sensor`, {
          cache: "no-store",
        });
        if (!response.ok) {
          throw new Error(`Device returned ${response.status}`);
        }

        const reading = (await response.json()) as SensorReading;
        if (cancelled) {
          return;
        }

        const readingWithTime = await evaluateReading({
          ...reading,
          createdAt: new Date().toISOString(),
        });

        setLatest(readingWithTime);
        setSeries((current) => [...current.slice(-39), readingWithTime]);
        setConnectionStatus("Connected");

        if (isSavingHistory) {
          await saveMeasurement(readingWithTime, false);
        }
      } catch (error) {
        if (!cancelled) {
          setConnectionStatus(error instanceof Error ? error.message : "Connection failed");
        }
      }
    }

    void poll();
    const interval = window.setInterval(poll, 3000);

    return () => {
      cancelled = true;
      window.clearInterval(interval);
    };
  }, [deviceUrl, isPolling, isSavingHistory, selectedCafeId, calibration]);

  async function loadCafes() {
    try {
      const response = await fetch("/api/cafes", { cache: "no-store" });
      if (!response.ok) {
        throw new Error("Database unavailable");
      }
      const data = (await response.json()) as { items: Cafe[] };
      if (data.items.length > 0) {
        setCafes(data.items);
        setSelectedCafeId(data.items[0].id);
      }
    } catch {
      setCafes([fallbackCafe]);
      setSelectedCafeId(fallbackCafe.id);
    }
  }

  async function loadHistory() {
    try {
      const response = await fetch("/api/history?take=60", { cache: "no-store" });
      if (!response.ok) {
        throw new Error("History unavailable");
      }
      const data = (await response.json()) as { items: HistoryItem[] };
      setHistory(data.items);
    } catch {
      setHistory([]);
    }
  }

  async function saveMeasurement(reading = latest, showNotice = true) {
    if (!reading) {
      return;
    }

    const cafeId = selectedCafeId === fallbackCafe.id ? undefined : selectedCafeId;
    const response = await fetch("/api/history", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ ...reading, cafeId }),
    });

    if (response.ok) {
      if (showNotice) {
        setNotice("Measurement saved to history.");
      }
      await loadHistory();
      return;
    }

    if (showNotice) {
      setNotice("History needs DATABASE_URL before it can save.");
    }
  }

  async function evaluateReading(reading: SensorReading) {
    const cafeId = selectedCafeId === fallbackCafe.id ? undefined : selectedCafeId;
    const response = await fetch("/api/evaluate", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({
        ...reading,
        cafeId,
        calibration,
      }),
    });

    if (!response.ok) {
      return {
        ...reading,
        status: "Unclassified",
      };
    }

    return (await response.json()) as SensorReading;
  }

  async function createCafe(event: FormEvent<HTMLFormElement>) {
    event.preventDefault();
    const response = await fetch("/api/cafes", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ name: newCafeName, location: newCafeLocation }),
    });

    if (!response.ok) {
      setNotice("Cafe storage needs DATABASE_URL.");
      return;
    }

    setNewCafeName("");
    setNewCafeLocation("");
    setNotice("Cafe profile created.");
    await loadCafes();
  }

  async function saveCalibration(event: FormEvent<HTMLFormElement>) {
    event.preventDefault();
    setNotice("");
    const messages: string[] = [];

    try {
      assertCalibration(calibration);
    } catch (error) {
      setNotice(error instanceof Error ? error.message : "Invalid calibration range.");
      return;
    }

    if (selectedCafeId !== fallbackCafe.id) {
      const response = await fetch("/api/calibration", {
        method: "PUT",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ ...calibration, cafeId: selectedCafeId }),
      });

      if (!response.ok) {
        setNotice("Calibration database save failed.");
        return;
      }

      await loadCafes();
      messages.push("Backend calibration updated.");
    }

    const normalizedDeviceUrl = normalizeDeviceUrl(deviceUrl);
    if (normalizedDeviceUrl) {
      try {
        const response = await fetch(`${normalizedDeviceUrl}/api/calibration`, {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify(calibration),
        });

        if (!response.ok) {
          throw new Error(`Device returned ${response.status}`);
        }

        messages.push("ESP32 calibration updated.");
      } catch (error) {
        setNotice(error instanceof Error ? error.message : "ESP32 calibration update failed.");
        return;
      }
    } else if (selectedCafeId === fallbackCafe.id) {
      messages.push("Local calibration updated. Set an ESP32 URL to save it on the device.");
    }

    setNotice(messages.join(" "));
  }

  function applyDeviceUrl(event: FormEvent<HTMLFormElement>) {
    event.preventDefault();
    const normalized = normalizeDeviceUrl(draftDeviceUrl);
    setDeviceUrl(normalized);
    localStorage.setItem("espressoDeviceUrl", normalized);
    setConnectionStatus(
      window.location.protocol === "https:" && normalized.startsWith("http://")
        ? "HTTPS dashboard cannot fetch an HTTP ESP URL."
        : normalized
          ? "Connecting"
          : "Not connected",
    );
  }

  function updateCalibration(key: keyof Calibration, value: string) {
    setCalibration((current) => ({
      ...current,
      [key]: Number(value),
    }));
  }

  const chartData = useMemo(
    () => ({
      labels: series.map((reading) =>
        reading.createdAt
          ? new Date(reading.createdAt).toLocaleTimeString([], {
              hour: "2-digit",
              minute: "2-digit",
              second: "2-digit",
            })
          : "",
      ),
      datasets: [
        {
          label: "TDS",
          data: series.map((reading) => reading.tds),
          borderColor: "#5b3a29",
          backgroundColor: "#5b3a29",
          tension: 0.35,
        },
        {
          label: "pH",
          data: series.map((reading) => reading.ph),
          borderColor: "#4d6575",
          backgroundColor: "#4d6575",
          tension: 0.35,
        },
        {
          label: "Temp C",
          data: series.map((reading) => reading.temperature),
          borderColor: "#a23f3f",
          backgroundColor: "#a23f3f",
          tension: 0.35,
        },
      ],
    }),
    [series],
  );

  const csv = useMemo(() => {
    const rows = [
      ["createdAt", "cafe", "tds", "ph", "temperature", "status"],
      ...history.map((item) => [
        item.createdAt ?? "",
        item.cafe?.name ?? "",
        String(item.tds),
        String(item.ph),
        String(item.temperature),
        item.status ?? "Unclassified",
      ]),
    ];

    return rows.map((row) => row.map((cell) => `"${cell.replaceAll('"', '""')}"`).join(",")).join("\n");
  }, [history]);

  return (
    <main className="min-h-screen">
      <header className="border-b border-stone-200 bg-paper/90">
        <div className="mx-auto flex max-w-7xl flex-col gap-4 px-4 py-5 sm:px-6 lg:flex-row lg:items-center lg:justify-between">
          <div>
            <p className="text-sm font-medium uppercase tracking-[0.18em] text-steel">
              Espresso Calibration
            </p>
            <h1 className="mt-1 text-3xl font-semibold text-ink">
              Coffee Quality Control
            </h1>
          </div>
          <form
            onSubmit={applyDeviceUrl}
            className="flex w-full flex-col gap-2 sm:flex-row lg:w-auto"
          >
            <label className="sr-only" htmlFor="device-url">
              ESP32 URL
            </label>
            <input
              id="device-url"
              value={draftDeviceUrl}
              onChange={(event) => setDraftDeviceUrl(event.target.value)}
              className="h-11 min-w-0 rounded-md border border-stone-300 bg-white px-3 text-sm shadow-panel sm:w-72"
              placeholder="http://192.168.4.1"
            />
            <button className="inline-flex h-11 items-center justify-center gap-2 rounded-md bg-ink px-4 text-sm font-medium text-white">
              <Wifi size={17} />
              Connect
            </button>
          </form>
        </div>
      </header>

      <div className="mx-auto grid max-w-7xl gap-5 px-4 py-5 sm:px-6 lg:grid-cols-[1.6fr_1fr]">
        <section className="space-y-5">
          <div className="grid gap-3 sm:grid-cols-2 xl:grid-cols-4">
            <MetricCard
              icon={<Gauge size={20} />}
              label="TDS"
              value={formatNumber(latest?.tds)}
              unit="%"
            />
            <MetricCard
              icon={<Activity size={20} />}
              label="pH"
              value={formatNumber(latest?.ph)}
              unit=""
            />
            <MetricCard
              icon={<Thermometer size={20} />}
              label="Temperature"
              value={formatNumber(latest?.temperature, 1)}
              unit="C"
            />
            <div className="rounded-md border border-stone-200 bg-white p-4 shadow-panel">
              <div className="flex items-center justify-between">
                <span className="text-sm text-stone-500">Status</span>
                <span className="text-stone-500">
                  <Activity size={20} />
                </span>
              </div>
              <div className="mt-5">
                <span
                  className={`inline-flex min-h-9 items-center rounded-md px-3 text-sm font-semibold ${statusClass(
                    latest?.status ?? "Unclassified",
                  )}`}
                >
                  {latest?.status ?? "Unclassified"}
                </span>
              </div>
            </div>
          </div>

          <section className="rounded-md border border-stone-200 bg-white p-4 shadow-panel">
            <div className="mb-4 flex flex-col gap-3 sm:flex-row sm:items-center sm:justify-between">
              <div>
                <h2 className="text-lg font-semibold text-ink">Realtime Monitoring</h2>
                <p className="text-sm text-stone-500">{connectionStatus}</p>
              </div>
              <div className="flex gap-2">
                <button
                  onClick={() => setIsPolling((value) => !value)}
                  className="inline-flex h-10 w-10 items-center justify-center rounded-md border border-stone-300 bg-white text-ink"
                  title={isPolling ? "Pause polling" : "Start polling"}
                >
                  {isPolling ? <Square size={17} /> : <Play size={17} />}
                </button>
                <button
                  onClick={() => void saveMeasurement()}
                  className="inline-flex h-10 w-10 items-center justify-center rounded-md border border-stone-300 bg-white text-ink"
                  title="Save current measurement"
                >
                  <Save size={17} />
                </button>
                <button
                  onClick={() => setIsSavingHistory((value) => !value)}
                  className={`inline-flex h-10 items-center justify-center gap-2 rounded-md px-3 text-sm font-medium ${
                    isSavingHistory ? "bg-sage text-white" : "border border-stone-300 bg-white text-ink"
                  }`}
                >
                  <Database size={17} />
                  Auto
                </button>
              </div>
            </div>
            <div className="h-[320px]">
              <Line
                data={chartData}
                options={{
                  maintainAspectRatio: false,
                  responsive: true,
                  plugins: {
                    legend: { position: "bottom" },
                  },
                  scales: {
                    x: { grid: { display: false } },
                    y: { beginAtZero: false },
                  },
                }}
              />
            </div>
          </section>

          <section className="rounded-md border border-stone-200 bg-white shadow-panel">
            <div className="flex items-center justify-between border-b border-stone-200 p-4">
              <div>
                <h2 className="text-lg font-semibold text-ink">History</h2>
                <p className="text-sm text-stone-500">Saved measurements from PostgreSQL</p>
              </div>
              <a
                href={`data:text/csv;charset=utf-8,${encodeURIComponent(csv)}`}
                download="espresso-history.csv"
                className="inline-flex h-10 w-10 items-center justify-center rounded-md border border-stone-300 bg-white text-ink"
                title="Export CSV"
              >
                <Download size={17} />
              </a>
            </div>
            <div className="overflow-x-auto">
              <table className="w-full min-w-[720px] text-left text-sm">
                <thead className="bg-stone-50 text-stone-500">
                  <tr>
                    <th className="px-4 py-3 font-medium">Time</th>
                    <th className="px-4 py-3 font-medium">Cafe</th>
                    <th className="px-4 py-3 font-medium">TDS</th>
                    <th className="px-4 py-3 font-medium">pH</th>
                    <th className="px-4 py-3 font-medium">Temp</th>
                    <th className="px-4 py-3 font-medium">Status</th>
                  </tr>
                </thead>
                <tbody className="divide-y divide-stone-100">
                  {history.length === 0 ? (
                    <tr>
                      <td className="px-4 py-6 text-stone-500" colSpan={6}>
                        No saved measurements yet.
                      </td>
                    </tr>
                  ) : (
                    history.map((item, index) => (
                      <tr key={item.id ?? `${item.createdAt}-${index}`}>
                        <td className="px-4 py-3">
                          {item.createdAt ? new Date(item.createdAt).toLocaleString() : "--"}
                        </td>
                        <td className="px-4 py-3">{item.cafe?.name ?? selectedCafe?.name ?? "--"}</td>
                        <td className="px-4 py-3">{formatNumber(item.tds)}</td>
                        <td className="px-4 py-3">{formatNumber(item.ph)}</td>
                        <td className="px-4 py-3">{formatNumber(item.temperature, 1)} C</td>
                        <td className="px-4 py-3">
                          <span
                            className={`inline-flex rounded-md px-2 py-1 text-xs font-semibold ${statusClass(
                              item.status ?? "Unclassified",
                            )}`}
                          >
                            {item.status ?? "Unclassified"}
                          </span>
                        </td>
                      </tr>
                    ))
                  )}
                </tbody>
              </table>
            </div>
          </section>
        </section>

        <aside className="space-y-5">
          <section className="rounded-md border border-stone-200 bg-white p-4 shadow-panel">
            <div className="mb-4 flex items-center gap-2">
              <Building2 size={19} />
              <h2 className="text-lg font-semibold text-ink">Cafe Profile</h2>
            </div>
            <label className="text-sm font-medium text-stone-600" htmlFor="cafe">
              Active cafe
            </label>
            <select
              id="cafe"
              value={selectedCafeId}
              onChange={(event) => setSelectedCafeId(event.target.value)}
              className="mt-2 h-11 w-full rounded-md border border-stone-300 bg-white px-3 text-sm"
            >
              {cafes.map((cafe) => (
                <option key={cafe.id} value={cafe.id}>
                  {cafe.name}
                </option>
              ))}
            </select>
            <form onSubmit={createCafe} className="mt-4 grid gap-3">
              <input
                value={newCafeName}
                onChange={(event) => setNewCafeName(event.target.value)}
                className="h-10 rounded-md border border-stone-300 px-3 text-sm"
                placeholder="New cafe name"
              />
              <input
                value={newCafeLocation}
                onChange={(event) => setNewCafeLocation(event.target.value)}
                className="h-10 rounded-md border border-stone-300 px-3 text-sm"
                placeholder="Location"
              />
              <button className="inline-flex h-10 items-center justify-center gap-2 rounded-md bg-espresso px-3 text-sm font-medium text-white">
                <Building2 size={17} />
                Add Cafe
              </button>
            </form>
          </section>

          <section className="rounded-md border border-stone-200 bg-white p-4 shadow-panel">
            <div className="mb-4 flex items-center gap-2">
              <Settings2 size={19} />
              <h2 className="text-lg font-semibold text-ink">Calibration</h2>
            </div>
            <form onSubmit={saveCalibration} className="grid gap-4">
              <RangeInputs
                label="TDS"
                min={calibration.tdsMin}
                max={calibration.tdsMax}
                minKey="tdsMin"
                maxKey="tdsMax"
                onChange={updateCalibration}
              />
              <RangeInputs
                label="pH"
                min={calibration.phMin}
                max={calibration.phMax}
                minKey="phMin"
                maxKey="phMax"
                onChange={updateCalibration}
              />
              <RangeInputs
                label="Temperature"
                min={calibration.tempMin}
                max={calibration.tempMax}
                minKey="tempMin"
                maxKey="tempMax"
                onChange={updateCalibration}
              />
              <label className="grid gap-2 text-sm font-medium text-stone-600">
                Tolerance
                <input
                  type="number"
                  step="0.01"
                  value={calibration.tolerance}
                  onChange={(event) => updateCalibration("tolerance", event.target.value)}
                  className="h-10 rounded-md border border-stone-300 px-3 text-sm"
                />
              </label>
              <button className="inline-flex h-11 items-center justify-center gap-2 rounded-md bg-ink px-3 text-sm font-medium text-white">
                <Save size={17} />
                Save Calibration
              </button>
            </form>
          </section>

          {notice ? (
            <div className="rounded-md border border-crema bg-white p-4 text-sm text-ink shadow-panel">
              {notice}
            </div>
          ) : null}
        </aside>
      </div>
    </main>
  );
}

function MetricCard({
  icon,
  label,
  value,
  unit,
}: {
  icon: React.ReactNode;
  label: string;
  value: string;
  unit: string;
}) {
  return (
    <div className="rounded-md border border-stone-200 bg-white p-4 shadow-panel">
      <div className="flex items-center justify-between">
        <span className="text-sm text-stone-500">{label}</span>
        <span className="text-stone-500">{icon}</span>
      </div>
      <div className="mt-4 flex items-end gap-2">
        <span className="text-3xl font-semibold text-ink">{value}</span>
        {unit ? <span className="pb-1 text-sm font-medium text-stone-500">{unit}</span> : null}
      </div>
    </div>
  );
}

function RangeInputs({
  label,
  min,
  max,
  minKey,
  maxKey,
  onChange,
}: {
  label: string;
  min: number;
  max: number;
  minKey: keyof Calibration;
  maxKey: keyof Calibration;
  onChange: (key: keyof Calibration, value: string) => void;
}) {
  return (
    <fieldset>
      <legend className="mb-2 text-sm font-medium text-stone-600">{label}</legend>
      <div className="grid grid-cols-2 gap-3">
        <label className="grid gap-1 text-xs font-medium uppercase text-stone-500">
          Min
          <input
            type="number"
            step="0.01"
            value={min}
            onChange={(event) => onChange(minKey, event.target.value)}
            className="h-10 rounded-md border border-stone-300 px-3 text-sm font-normal text-ink"
          />
        </label>
        <label className="grid gap-1 text-xs font-medium uppercase text-stone-500">
          Max
          <input
            type="number"
            step="0.01"
            value={max}
            onChange={(event) => onChange(maxKey, event.target.value)}
            className="h-10 rounded-md border border-stone-300 px-3 text-sm font-normal text-ink"
          />
        </label>
      </div>
    </fieldset>
  );
}
