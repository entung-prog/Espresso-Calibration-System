export type Calibration = {
  tdsMin: number;
  tdsMax: number;
  phMin: number;
  phMax: number;
  tempMin: number;
  tempMax: number;
  tolerance: number;
};

export type SensorReading = {
  temperature: number;
  ph: number;
  tds: number;
  tdsPpm?: number;
  status?: string;
  createdAt?: string;
};

export type Cafe = {
  id: string;
  name: string;
  location?: string | null;
  calibration?: Calibration | null;
};
