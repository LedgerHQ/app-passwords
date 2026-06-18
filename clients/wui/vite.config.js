import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";
import { nodePolyfills } from "vite-plugin-node-polyfills";

// https://vite.dev/config/
export default defineConfig({
  // Served from a dedicated domain (passwords.ledger.com), so assets live at
  // the site root.
  base: "/",
  // Open the browser automatically when the dev server starts.
  server: { open: true },
  plugins: [
    react(),
    // The APDU layer (PasswordsManager) uses the Node Buffer API; expose a
    // browser polyfill.
    nodePolyfills({ include: ["buffer"], globals: { Buffer: true } }),
  ],
  test: {
    globals: true,
    environment: "jsdom",
  },
});
