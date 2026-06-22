# Passwords Backup — Web UI

[Live demo here](https://passwords.ledger.com)

A browser-based tool to **backup and restore** the list of `password nicknames`
stored inside the **Passwords app** on your Ledger device. Everything runs
locally in your browser; the backup file never leaves your machine.

Built with **Vite + React 18** and Ledger's **lumen** design system
(`@ledgerhq/lumen-ui-react`, Tailwind CSS).

## What it does

A backup is useful because the nickname list is erased when you update the
Passwords app or the device firmware. It is also needed to fully recover your
passwords after losing a device: restoring the
[24-words recovery phrase](https://www.ledger.com/academy/crypto/what-is-a-recovery-phrase)
is necessary but not sufficient — you also need your nickname list.

The backup is a human-readable `passwords_backup.json` file containing a dump of
the application storage. It holds only nicknames (no secrets), so it is not
confidential.

For more information on the device application itself, see the
[app README](https://github.com/LedgerHQ/app-passwords/blob/master/README.md).

## Requirements

- A **Chromium-based browser** (Chrome, Chromium, Brave, Edge). WebUSB is not
  available in Firefox or Safari. On Windows, enable the new USB backend at
  `chrome://flags` if the connection fails.
- The **Passwords app open** on a connected Ledger device.

## Usage

- Open the **Passwords app** on your device.
- Click **Connect** — on success the Backup / Restore actions appear.
  (Connection trouble? See
  [Fix connection issues](https://support.ledger.com/article/115005165269-zd).)
- **Backup** prompts "Backup password list?" on the device, then saves the file.
- **Restore** asks for a previous backup file, then prompts "Restore password
  list?" on the device.

## Develop

This project uses **pnpm** (provisioned by corepack from the `packageManager`
field — run `corepack enable` once if needed) and **Node 22**:

```sh
pnpm install
pnpm dev        # Vite dev server (opens the browser)
pnpm mock       # same, but opens at /?mock — preview the connected screen with no device
pnpm build      # production bundle in dist/
pnpm preview    # serve the production build locally
```

### Preview without a device (`?mock`)

Append `?mock` to the URL (or run `pnpm mock`) to jump straight to the
*connected* screen so the layout can be reviewed without a Ledger. It is
**dev-only** (`import.meta.env.DEV` gates it); production builds ignore the
flag. Backup/Restore still fail without a device, but every screen renders.

### Tests

The pure-logic units (charset mapping, metadata serialization round-trip) are
covered by **Vitest** and run without a device:

```sh
pnpm test        # run once (CI)
pnpm test:watch  # watch mode
```

## Deployment

Built as a static site and published to GitHub Pages on the dedicated domain
**passwords.ledger.com** (see `public/CNAME`). The `wui.yml` workflow builds
`dist/` and deploys it with `peaceiris/actions-gh-pages` on pushes to
`develop`/`master`. The Vite `base` is `/` (assets at the site root).
