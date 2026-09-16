import { ledgerLivePreset } from "@ledgerhq/lumen-design-core";

/** @type {import('tailwindcss').Config} */
export default {
  presets: [ledgerLivePreset],
  theme: {
    extend: {
      // The lumen preset sets `fontFamily: "Inter"` with nothing after it, so a page that fails
      // to load the bundled woff2 falls back to whatever the browser defaults to. Name a real
      // stack instead. Worth reporting upstream: the preset should ship its own fallbacks.
      fontFamily: {
        sans: [
          "Inter Variable",
          "Inter",
          "system-ui",
          "-apple-system",
          "Segoe UI",
          "Helvetica Neue",
          "Arial",
          "sans-serif",
        ],
      },
    },
  },
  content: [
    "./index.html",
    "./src/**/*.{js,jsx}",
    // Scan lumen's compiled components so Tailwind emits the classes they use.
    "./node_modules/@ledgerhq/lumen-ui-react/dist/**/*.js",
  ],
};
