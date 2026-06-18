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

async function saveJSON(payload, suggestedName) {
  const text = JSON.stringify(payload, null, 4);

  if (window.showSaveFilePicker) {
    let handle;
    try {
      handle = await window.showSaveFilePicker({
        suggestedName,
        types: [
          { description: "Passwords backup", accept: { "application/json": [".json"] } },
        ],
      });
    } catch (error) {
      if (error.name === "AbortError") return false;
      throw error;
    }
    const writable = await handle.createWritable();
    await writable.write(text);
    await writable.close();
    return true;
  }

  const blob = new Blob([text], { type: "application/json;charset=utf-8" });
  const url = URL.createObjectURL(blob);
  const a = document.createElement("a");
  a.href = url;
  a.download = suggestedName;
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
  const [storageSize, setStorageSize] = useState(mock ? 4096 : null);
  const [notice, setNotice] = useState(null);

  const fileInput = useRef(null);

  useEffect(() => {
    passwords.onDisconnect = () => {
      setConnected(false);
      setBusy(false);
      setVersion(null);
      setStorageSize(null);
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
      setStorageSize(passwords.storage_size);
      setConnected(true);
      setNotice({ appearance: "success", title: "Device connected" });
    } catch (error) {
      await passwords.disconnect();
      setConnected(false);
      setVersion(null);
      setStorageSize(null);
      setNotice({ appearance: "error", title: "Connection failed", description: String(error) });
    } finally {
      setBusy(false);
    }
  }

  async function onDisconnect() {
    await passwords.disconnect();
    setConnected(false);
    setVersion(null);
    setStorageSize(null);
    setNotice({ appearance: "info", title: "Disconnected" });
  }

  async function onBackup() {
    setBusy(true);
    setNotice({ appearance: "info", title: 'Approve "Transfer metadatas?" on your device' });
    try {
      const payload = await passwords.dump_metadatas();
      const saved = await saveJSON(payload, "passwords_backup.json");
      setNotice(
        saved
          ? { appearance: "success", title: "Backup saved" }
          : { appearance: "info", title: "Backup cancelled" }
      );
    } catch (error) {
      setNotice({ appearance: "error", title: "Backup failed", description: String(error) });
    } finally {
      setBusy(false);
    }
  }

  function onPickRestoreFile(event) {
    const file = event.target.files[0];
    event.target.value = "";
    if (!file) return;
    const reader = new FileReader();
    reader.onload = async () => {
      setBusy(true);
      setNotice({ appearance: "info", title: 'Approve "Overwrite metadatas?" on your device' });
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
                    <div className="flex items-center justify-between">
                      <span className="body-3 text-muted">Storage</span>
                      <span className="body-3 text-base">
                        {storageSize ? `${storageSize} bytes` : "—"}
                      </span>
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
