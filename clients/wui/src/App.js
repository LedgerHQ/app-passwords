import React, { useState } from "react";
import logo from "./logo.svg";
import RestoreButton from "./components/RestoreButton";
import BackupButton from "./components/BackupButton";
import PasswordsManager from "./controller/PasswordsManager.js";
import AppExplanations from "./components/AppExplanations";
import { ToastContainer, toast } from "react-toastify";
import "react-toastify/dist/ReactToastify.css";
import { listen } from "@ledgerhq/logs";
import packageJson from "../package.json";
import "./App.css";

const passwordsManager = new PasswordsManager();
listen((log) => {
  console.log(log);
});

function App() {
  function ask_device(device_handler, request) {
    return new Promise(async (resolve) => {
      let result = null;
      try {
        if (!device_handler.connected) {
          await device_handler.connect();
          setConnected(true);
          toast.info("Device connected 👌", { autoClose: false });
        }
        if (request) {
          toast.info("Approve action on your device ✨", { autoClose: false });
          setBusy(true);
          result = await request();

          toast.dismiss();
          toast.success("Success 🦄");
        }
      } catch (error) {
        device_handler.disconnect();
        setConnected(false);
        toast.dismiss();
        toast.error(`${error.toString()} 🙅`);
      } finally {
        setBusy(false);
        resolve(result);
      }
    });
  }

  // const  = new PasswordsManager(true);

  const [isBusy, setBusy] = useState(false);
  const [isConnected, setConnected] = useState(false);

  return (
    <div className="App">
      <ToastContainer hideProgressBar={true} />
      <div className="App-banner">
        <img src={logo} className="App-logo" alt="logo" />
        <p className="App-title">Passwords Backup</p>
      </div>
      <header className="App-header">
        <div className="Commands">
          <BackupButton
            text="Connect"
            color="#41ccb4"
            disabled={isBusy}
            hidden={isConnected}
            onClick={() => ask_device(passwordsManager)}
          />
          <BackupButton
            text="Backup"
            color="#41ccb4"
            disabled={isBusy}
            hidden={!isConnected}
            onClick={() =>
              ask_device(passwordsManager, () =>
                passwordsManager.dump_metadatas()
              )
            }
          />
          <RestoreButton
            text="Restore"
            color="#FFB86D"
            disabled={isBusy}
            hidden={!isConnected}
            onClick={(metadatas) =>
              ask_device(passwordsManager, () =>
                passwordsManager.load_metadatas(metadatas)
              )
            }
          />
        </div>
        <AppExplanations></AppExplanations>
        <div className="App-footer">
          <p>
            A modest Web App built at Ledger with React, hosted by Github. v
            {`${packageJson.version}`}.{" "}
            <a href="https://github.com/LedgerHQ/passwords-backup">
              PRs welcomed and appreciated ✨
            </a>
          </p>
        </div>
      </header>
    </div>
  );
}

export default App;
