import { useState } from "react";
import ReactMarkdown from "react-markdown";
import { ChevronDown } from "@ledgerhq/lumen-ui-react/symbols";

// FAQ content preserved from the original web UI. Markdown is rendered with
// react-markdown; lumen typography tokens style the surrounding chrome.
const ITEMS = [
  {
    q: "What is this Web App?",
    a:
      "This Web App allows you to backup/restore the list of `password nicknames` stored inside the `Passwords app` on your Ledger device.\n\n" +
      "It is useful to have such a backup when you update the Passwords app on your device, or the device firmware, because the list gets erased. Another case where it's practical to have a nickname backup is when you lose your device: Restoring the [24-words recovery phrase](https://www.ledger.com/academy/crypto/what-is-a-recovery-phrase) is necessary but not sufficient to restore your passwords, you need your nickname list as well.\n\n" +
      "The backup consists in a human readable `backup.json` file containing a dump of the application storage.\n\n" +
      "Note that all operations of this Web App are done locally on your computer, there are no external communications occurring.",
  },
  {
    q: "What is the Ledger Passwords application?",
    a: "Look [here](https://github.com/LedgerHQ/app-passwords/blob/master/README.md) for more information on the device application itself.",
  },
  {
    q: "How to use this Web App?",
    a:
      "* Connect your Ledger device to your computer and open the `Passwords app`.\n" +
      "* Click on the big `Connect` button — on success the `Backup` and `Restore` buttons appear. If you have trouble with this step, have a look [here](https://support.ledger.com/article/115005165269-zd).\n" +
      "* `Backup` will prompt a screen requesting your approval on your device (\"Transfer metadatas?\"), then save a backup file. This backup is not confidential, so you can for instance e-mail it to yourself to never lose it.\n" +
      "* `Restore` will prompt a file input dialog where you should indicate a previous backup file. A prompt (\"Overwrite metadatas?\") will then request your approval on your device.",
  },
  {
    q: "Which web browsers and operating systems are supported?",
    a: "The communication with the device is done through `WebUSB`, which is currently supported only on `Google Chrome` / `Chromium` / `Brave` for `Linux` and `MacOS`. On `Windows`, you need to first go to `chrome://flags`, search for `Enable new USB backend`, disable it and relaunch Chrome.",
  },
  {
    q: "Less common use cases",
    a:
      "* If you ever encounter a WTF-kind of error with your passwords app (some or all of your entries are suddenly gone? A password has changed?), it is wise to first come here and make a backup. You can then have a look inside the backup file to see if something is wrong (you might also want to create an issue [here](https://github.com/LedgerHQ/app-passwords/issues) so we fix it for all users).\n" +
      "* If you want to add a lot of new passwords, the manual input on the device keyboard will show its limits. You can instead create a backup, edit it manually to add all your new entries, then restore your app with this file.",
  },
];

function FaqItem({ q, a, open, onToggle }) {
  return (
    <div className="border-b border-muted last:border-b-0">
      <button
        type="button"
        onClick={onToggle}
        aria-expanded={open}
        className="flex w-full items-center justify-between gap-12 py-16 text-left"
      >
        <span className="body-1-semi-bold text-base">{q}</span>
        <ChevronDown
          size={20}
          className={`text-muted shrink-0 transition-transform ${open ? "rotate-180" : ""}`}
        />
      </button>
      {open && (
        <div className="markdown body-2 text-muted flex flex-col gap-8 pb-16">
          <ReactMarkdown>{a}</ReactMarkdown>
        </div>
      )}
    </div>
  );
}

export default function Faq() {
  const [openIndex, setOpenIndex] = useState(null);

  return (
    <div className="bg-base border border-base rounded-lg px-20">
      {ITEMS.map((item, i) => (
        <FaqItem
          key={i}
          q={item.q}
          a={item.a}
          open={openIndex === i}
          onToggle={() => setOpenIndex(openIndex === i ? null : i)}
        />
      ))}
    </div>
  );
}
