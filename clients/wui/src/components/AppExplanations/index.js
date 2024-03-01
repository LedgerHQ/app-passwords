import React, { useState } from "react";

import {
  Accordion,
  AccordionItem,
  AccordionItemHeading,
  AccordionItemButton,
  AccordionItemPanel,
} from "react-accessible-accordion";
import ReactMarkdown from "react-markdown";
import "./index.css";

export default function AppExplanations() {
  const [expandedItems, setexpandedItems] = useState([]);

  // In case the user expands a node that is barely visible, we scroll the page to display it fully
  function handleExpand(update) {
    if (update.length > expandedItems.length) {
      const newExpandedItemUUID = update[update.length - 1];
      const itemButtonBottom = document
        .getElementById(`accordion__panel-${newExpandedItemUUID}`)
        .getBoundingClientRect().bottom;
      if (itemButtonBottom > window.innerHeight) {
        window.scrollBy(0, itemButtonBottom - window.innerHeight);
      }
    }
    setexpandedItems(update);
  }

  const whatIsThisWebApp_help =
    // eslint-disable-next-line
    "This Web App allows you to backup/restore the list of `password nicknames` stored inside the `Passwords app` on your Ledger Nano S/ Nano X.  \n\
    It is useful to have such a backup when you update the Passwords app on your device, or the device firmware, because the list gets erased. Another case where it's practical to have a nickname backup is when you loose your device: Restoring the [24-words recovery phrase](https://www.ledger.com/academy/crypto/what-is-a-recovery-phrase) is necessary but not sufficient to restore your passwords, you need your nickname list as well.  \n\
    The backup consists in a human readable `backup.json` file containing a dump of the 4096 bytes of application storage.  \n\
    Note that all operations of this Web App are done locally on your computer, there are no external communications occurring.";

  const whatIsTheLedgerPasswordsApp_help =
    "Look [here](https://github.com/LedgerHQ/app-passwords/blob/master/README.md) for more information on the device application itself.";

  const howToUseThisWebApp_help =
    // eslint-disable-next-line
    '* Connect your Nano S/X to your computer and open the `Passwords app`.\n* You can now click on the big `Connect` button, and if it succeeds the `Backup` and `Restore` buttons should replace the previous button. If you have troubles with this step, have a look [here](https://support.ledger.com/hc/en-us/articles/115005165269-Fix-connection-issues). \n* Either click on `Backup` or `Restore` depending on what you want to do:  \n\
    * `Backup` will prompt a screen requesting your approval on your device (`"Transfer metadatas ?"`), then save a backup file. This is your backup. it\'s not confidential, so for instance you can send it to yourself by e-mail to never loose it.  \n\
    * `Restore` will prompt a file input dialog where you should indicate a previous backup file. A prompt (`"Overwrite metadatas ?"`) will then request your approval on your device. Done.';

  const whichbrowsersAreSupported_help =
    "The communication with the device is done through `WebUSB`, which is currently supported only on `Google Chrome` / `Chromium` / `Brave` for `Linux` and `MacOS`. On `Windows`, you need to first go to `chrome://flags` then search for `Enable new USB backend`, disable it and relaunch Chrome.";

  const lessCommonUseCases_help =
    // eslint-disable-next-line
    "* If you ever encounter a WTF-kind of error with your passwords app (some or all of your entries are suddenly gone? A password has changed ?), it is wise to first come here and make a backup. You can then have a look inside the backup file to see if something is wrong (You might also want to create an issue [here](https://github.com/LedgerHQ/app-passwords/issues) so we fix your issue for all users). \n* If you want to add a lot of new passwords, the manual input on the device keyboard will show its limits. You can instead create a backup and edit it manually to add all your new entries. You just have to restore your app with this file and the job is done :)";

  return (
    <Accordion allowZeroExpanded allowMultipleExpanded onChange={handleExpand}>
      <AccordionItem>
        <AccordionItemHeading>
          <AccordionItemButton>What is this Web App ?</AccordionItemButton>
        </AccordionItemHeading>
        <AccordionItemPanel>
          <ReactMarkdown
            className="Explanations"
            children={whatIsThisWebApp_help}
          ></ReactMarkdown>
        </AccordionItemPanel>
      </AccordionItem>
      <AccordionItem>
        <AccordionItemHeading>
          <AccordionItemButton>
            What is the Ledger Passwords application ?
          </AccordionItemButton>
        </AccordionItemHeading>
        <AccordionItemPanel>
          <ReactMarkdown
            className="Explanations"
            children={whatIsTheLedgerPasswordsApp_help}
          ></ReactMarkdown>
        </AccordionItemPanel>
      </AccordionItem>
      <AccordionItem>
        <AccordionItemHeading>
          <AccordionItemButton>How to use this Web App ?</AccordionItemButton>
        </AccordionItemHeading>
        <AccordionItemPanel>
          <ReactMarkdown
            className="Explanations"
            children={howToUseThisWebApp_help}
          ></ReactMarkdown>
        </AccordionItemPanel>
      </AccordionItem>
      <AccordionItem>
        <AccordionItemHeading>
          <AccordionItemButton>
            Which web browsers and operating systems are supported ?
          </AccordionItemButton>
        </AccordionItemHeading>
        <AccordionItemPanel>
          <ReactMarkdown
            className="Explanations"
            children={whichbrowsersAreSupported_help}
          ></ReactMarkdown>
        </AccordionItemPanel>
      </AccordionItem>
      <AccordionItem>
        <AccordionItemHeading>
          <AccordionItemButton>Less common use cases</AccordionItemButton>
        </AccordionItemHeading>
        <AccordionItemPanel>
          <ReactMarkdown
            className="Explanations"
            children={lessCommonUseCases_help}
          ></ReactMarkdown>
        </AccordionItemPanel>
      </AccordionItem>
    </Accordion>
  );
}
