// Redaction for @ledgerhq/logs events before they reach a console.
//
// `hw-transport` logs whole APDU frames as `log("apdu", "=> " + apdu.toString("hex"))`, so the
// bytes land in `message`, not in `data`. During backup and restore those bytes are the password
// nicknames, and a browser console ends up in screenshots, support bundles and remote debugging
// sessions. Keep the direction and the frame size, which is what makes the log useful when
// debugging a transfer, and drop the bytes themselves.
export function redactLogEvent({ type, message, data, ...rest }) {
  void data; // the documented payload field; never printed

  if (type === "apdu") {
    const [direction, hex = ""] = String(message ?? "").split(" ");
    return { type, direction, bytes: Math.floor(hex.length / 2), ...rest };
  }

  return { type, message, ...rest };
}
