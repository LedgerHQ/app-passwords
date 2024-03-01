import React, { useState, useCallback } from "react";
import "./index.css";

function downloadFile(fileData) {
  var blob = new Blob([JSON.stringify(fileData, null, 4)], {
    type: "application/json;charset=utf-8",
  });
  var url = URL.createObjectURL(blob);
  var elem = document.createElement("a");
  elem.href = url;
  elem.download = "backup.json";
  document.body.appendChild(elem);
  elem.click();
  document.body.removeChild(elem);
}

function BackupButton({ text, color, disabled, hidden, onClick }) {
  const [isLoading, setLoading] = useState(false);

  const onTriggerRunThenSaveFile = useCallback(() => {
    setLoading(true);
    onClick().then((fileData) => {
      if (fileData) downloadFile(fileData);
      setLoading(false);
    });
  }, [onClick]);

  return (
    <button
      className="BackupButton"
      disabled={isLoading | disabled}
      onClick={isLoading ? null : onTriggerRunThenSaveFile}
      style={{
        margin: "10px",
        backgroundColor: color,
        display: hidden ? "none" : true,
      }}
    >
      {isLoading ? "Loading…" : text}
    </button>
  );
}

export default BackupButton;
