import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";

// https://vite.dev/config/
export default defineConfig({
  // Served from a dedicated domain (passwords.ledger.com), so assets live at
  // the site root.
  base: "/",
  // Open the browser automatically when the dev server starts.
  server: { open: true },
  plugins: [react()],
  test: {
    globals: true,
    environment: "jsdom",
  },
});
