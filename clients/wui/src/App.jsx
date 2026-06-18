import { useEffect, useRef, useState } from "react";
import {
  ThemeProvider,
  TooltipProvider,
  Button,
  Banner,
  Tag,
  Divider,
  Spinner,
  Link,
} from "@ledgerhq/lumen-ui-react";
import {
  CloudDownload,
  CloudUpload,
  Unlink,
  Usb,
  ShieldCheck,
  CheckmarkCircleFill,
} from "@ledgerhq/lumen-ui-react/symbols";
import { listen } from "@ledgerhq/logs";
import PasswordsManager from "./controller/PasswordsManager.js";
import Faq from "./components/Faq.jsx";
import logo from "./assets/logo-padlock.png";
import packageJson from "../package.json";

const passwords = new PasswordsManager();
listen((log) => console.log(log));

// Save the backup, preferring the native "Save As" dialog so the user can
// choose the file name/location. showSaveFilePicker requires transient user
// activation, so this MUST be called from its own click handler — not from the
// async continuation of the device exchange (the original activation would have
// expired -> "Must be handling a user gesture"). Hence the two-step flow:
// backup first, then a separate "Save" click. Returns false if the user
// cancels the dialog (so the caller can keep the data for a retry). Falls back
// to a plain anchor download where the File System Access API is unavailable.
async function saveBackupFile(payload, filename) {
  const text = JSON.stringify(payload, null, 4);

  if (window.showSaveFilePicker) {
    let handle = null;
    try {
      handle = await window.showSaveFilePicker({
        suggestedName: filename,
        types: [
          { description: "Passwords backup", accept: { "application/json": [".json"] } },
        ],
      });
    } catch (error) {
      if (error.name === "AbortError") return false; // user dismissed the dialog
      // Other errors (unsupported / SecurityError): fall through to the anchor.
    }
    if (handle) {
      const writable = await handle.createWritable();
      await writable.write(text);
      await writable.close();
      return true;
    }
  }

  const blob = new Blob([text], { type: "application/json;charset=utf-8" });
  const url = URL.createObjectURL(blob);
  const a = document.createElement("a");
  a.href = url;
  a.download = filename;
  document.body.appendChild(a);
  a.click();
  document.body.removeChild(a);
  URL.revokeObjectURL(url);
  return true;
}

export default function App() {
  const mock =
    import.meta.env.DEV &&
    new URLSearchParams(window.location.search).has("mock");

  const [busy, setBusy] = useState(false);
  const [connected, setConnected] = useState(mock);
  const [version, setVersion] = useState(mock ? "1.0.0" : null);
  const [notice, setNotice] = useState(null);
  // Backup data read from the card, waiting for the user to pick a file. The
  // save dialog needs its own click (transient activation), so it can't be
  // chained onto the device exchange.
  const [pendingBackup, setPendingBackup] = useState(null);

  const fileInput = useRef(null);

  useEffect(() => {
    passwords.onDisconnect = () => {
      setConnected(false);
      setBusy(false);
      setVersion(null);
      setNotice({
        appearance: "warning",
        title: "Device disconnected",
        description: "Reconnect to continue.",
      });
    };
    return () => {
      passwords.onDisconnect = null;
    };
  }, []);

  async function onConnect() {
    setBusy(true);
    try {
      await passwords.connect();
      setVersion(passwords.version);
      setConnected(true);
      setNotice({ appearance: "success", title: "Device connected" });
    } catch (error) {
      await passwords.disconnect();
      setConnected(false);
      setVersion(null);
      setNotice({ appearance: "error", title: "Connection failed", description: String(error) });
    } finally {
      setBusy(false);
    }
  }

  async function onDisconnect() {
    await passwords.disconnect();
    setConnected(false);
    setVersion(null);
    setNotice({ appearance: "info", title: "Disconnected" });
  }

  async function onBackup() {
    setBusy(true);
    setPendingBackup(null);
    setNotice({ appearance: "info", title: 'Approve "Backup password list?" on your device' });
    try {
      // Read the card first. The file dialog can't be opened here (the device
      // approval consumed the click's activation), so hold the data and let the
      // user trigger the save with a fresh click.
      const payload = await passwords.dump_metadatas();
      setPendingBackup(payload);
      setNotice({
        appearance: "success",
        title: "Backup ready",
        description: "Click Save to choose where to store the file.",
      });
    } catch (error) {
      setNotice({ appearance: "error", title: "Backup failed", description: String(error) });
    } finally {
      setBusy(false);
    }
  }

  async function onSaveBackup() {
    try {
      const saved = await saveBackupFile(pendingBackup, "passwords_backup.json");
      if (saved) {
        setPendingBackup(null);
        setNotice({ appearance: "success", title: "Backup saved" });
      }
      // If cancelled, keep pendingBackup so the user can try saving again.
    } catch (error) {
      setNotice({ appearance: "error", title: "Save failed", description: String(error) });
    }
  }

  function onPickRestoreFile(event) {
    const file = event.target.files[0];
    event.target.value = "";
    if (!file) return;
    const reader = new FileReader();
    reader.onload = async () => {
      setBusy(true);
      setNotice({ appearance: "info", title: 'Approve "Restore password list?" on your device' });
      try {
        await passwords.load_metadatas(reader.result);
        setNotice({ appearance: "success", title: "Restore complete" });
      } catch (error) {
        setNotice({ appearance: "error", title: "Restore failed", description: String(error) });
      } finally {
        setBusy(false);
      }
    };
    reader.readAsText(file);
  }

  return (
    <ThemeProvider colorScheme="dark">
      <TooltipProvider>
        <div className="flex min-h-screen flex-col items-center bg-canvas px-16 py-32">
          {/* Header */}
          <div className="flex flex-col items-center gap-12 mb-32">
            <img src={logo} alt="" className="size-48" />
            <h1 className="heading-2-semi-bold text-base">Passwords Backup &amp; Restore</h1>
            <p className="body-2 text-muted text-center">
              Backup and restore the password nicknames stored in your Ledger Passwords app.
            </p>
          </div>

          {/* Content area */}
          <div className="flex w-full max-w-560 flex-col gap-20">
            {/* Notice banner */}
            {notice && (
              <Banner
                appearance={notice.appearance}
                title={notice.title}
                description={notice.description}
                onClose={() => setNotice(null)}
              />
            )}

            {/* Backup ready: the save dialog needs its own click. */}
            {pendingBackup && (
              <Banner
                appearance="info"
                title="Backup ready to save"
                description="Choose where to store your backup file."
                primaryAction={
                  <Button appearance="accent" size="sm" icon={CloudDownload} onClick={onSaveBackup}>
                    Save…
                  </Button>
                }
              />
            )}

            {/* Device card */}
            <div className="bg-base border border-base rounded-lg p-20">
              <div className="flex items-center justify-between">
                <div className="flex items-center gap-12">
                  <Usb size={20} className="text-muted" />
                  <span className="body-2-semi-bold text-base">Device</span>
                </div>
                {connected ? (
                  <Tag appearance="success" icon={CheckmarkCircleFill} label="Connected" />
                ) : (
                  <Tag appearance="gray" label="Not connected" />
                )}
              </div>

              {connected && (
                <>
                  <Divider className="my-16" />
                  <div className="flex flex-col gap-8">
                    <div className="flex items-center justify-between">
                      <span className="body-3 text-muted">App</span>
                      <span className="body-3 text-base">Passwords</span>
                    </div>
                    <div className="flex items-center justify-between">
                      <span className="body-3 text-muted">Version</span>
                      <Tag appearance="gray" label={version || "—"} />
                    </div>
                  </div>
                </>
              )}
            </div>

            {/* Actions card */}
            <div className="bg-base border border-base rounded-lg p-20">
              <div className="flex items-center gap-12 mb-16">
                <ShieldCheck size={20} className="text-muted" />
                <span className="body-2-semi-bold text-base">Actions</span>
              </div>
              <Divider className="mb-16" />

              {!connected && (
                <div className="flex flex-col items-center gap-16 py-24">
                  {busy ? (
                    <Spinner size={32} />
                  ) : (
                    <>
                      <p className="body-2 text-muted text-center">
                        Connect your Ledger with the Passwords app open.
                      </p>
                      <Button appearance="accent" icon={Usb} disabled={busy} loading={busy} onClick={onConnect}>
                        Connect
                      </Button>
                    </>
                  )}
                </div>
              )}

              {connected && (
                <div className="flex flex-col gap-10">
                  <Button appearance="accent" icon={CloudDownload} disabled={busy} loading={busy} onClick={onBackup}>
                    Backup
                  </Button>
                  <Button
                    appearance="base"
                    icon={CloudUpload}
                    disabled={busy}
                    onClick={() => fileInput.current.click()}
                  >
                    Restore
                  </Button>
                  <Divider className="my-6" />
                  <Button appearance="gray" icon={Unlink} disabled={busy} onClick={onDisconnect}>
                    Disconnect
                  </Button>
                </div>
              )}

              <input
                ref={fileInput}
                type="file"
                accept=".json,application/json"
                className="hidden"
                onChange={onPickRestoreFile}
              />
            </div>

            {/* Info banner */}
            <Banner
              appearance="info"
              title="Everything stays on your machine"
              description="The backup file holds only your password nicknames (no secrets). All operations run locally — nothing is sent to any server."
            />

            {/* FAQ */}
            <Faq />
          </div>

          {/* Footer */}
          <div className="mt-auto pt-40 text-center">
            <p className="body-3 text-muted">
              Built at Ledger with React &amp; lumen, hosted on GitHub. v{packageJson.version}.{" "}
              <Link href="https://github.com/LedgerHQ/app-passwords">PRs welcome ✨</Link>
            </p>
            <p className="body-4 text-muted-subtle mt-4">
              Requires a Chromium-based browser with WebUSB.
            </p>
          </div>
        </div>
      </TooltipProvider>
    </ThemeProvider>
  );
}
