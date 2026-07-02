import type { Config } from "tailwindcss";

const config: Config = {
  content: ["./app/**/*.{ts,tsx}", "./components/**/*.{ts,tsx}"],
  theme: {
    extend: {
      colors: {
        ink: "#171512",
        paper: "#fbfaf7",
        espresso: "#5b3a29",
        crema: "#d7b16a",
        sage: "#6f8067",
        steel: "#4d6575",
        cherry: "#a23f3f",
      },
      boxShadow: {
        panel: "0 1px 2px rgba(23, 21, 18, 0.06)",
      },
    },
  },
  plugins: [],
};

export default config;
