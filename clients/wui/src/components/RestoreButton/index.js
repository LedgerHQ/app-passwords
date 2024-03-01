import React, { useRef, useCallback, useState } from "react";
import "./index.css";

function RestoreButton({ text, color, disabled, hidden, onClick }) {
  const [isLoading, setLoading] = useState(false);
  const file = useRef(null);

  const hasFileInputBeenCanceled = () => {
    if (!file.current.value.length) setLoading(false);
    document.body.onfocus = null;
  };

  const onTriggerFileSelect = useCallback(() => {
    setLoading(true);
    document.body.onfocus = hasFileInputBeenCanceled;
    file.current && file.current.click();
  }, []);

  const onSelectedFileChanged = useCallback(
    (event) => {
      event.target.files[0].text().then((text) => {
        event.target.value = "";
        onClick(text).then(() => setLoading(false));
      });
    },
    [onClick]
  );

  return (
    <button
      className="RestoreButton"
      disabled={isLoading | disabled}
      onClick={isLoading ? null : onTriggerFileSelect}
      style={{
        margin: "10px",
        backgroundColor: color,
        display: hidden ? "none" : true,
      }}
    >
      {isLoading ? "Loading…" : text}
      <input
        type="file"
        ref={file}
        onChange={onSelectedFileChanged}
        accept=".json"
        style={{ display: "none" }}
      />
    </button>
  );
}

export default RestoreButton;
