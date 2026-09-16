import { redactLogEvent } from "./logRedaction.js";

// A real restore frame: LOAD_METADATAS carrying the nickname "allah" in the clear.
const RESTORE_FRAME = "e005ff000c02000761060007616c6c6168";

describe("redactLogEvent", () => {
  test("drops the APDU bytes, which is where the nicknames are", () => {
    const out = redactLogEvent({
      type: "apdu",
      message: `=> ${RESTORE_FRAME}`,
      id: "1",
      date: new Date(0),
    });

    // The frame contains "allah" as 616c6c6168; nothing hex-like may survive.
    const printed = JSON.stringify(out);
    expect(printed).not.toContain("616c6c6168");
    expect(printed).not.toContain(RESTORE_FRAME);
    // Direction and size are what make the log useful, and they leak nothing.
    expect(out).toMatchObject({ type: "apdu", direction: "=>", bytes: 17 });
  });

  test("redacts the response direction too", () => {
    const out = redactLogEvent({ type: "apdu", message: "<= 0a0007706173730000" });
    expect(JSON.stringify(out)).not.toContain("0a0007");
    expect(out).toMatchObject({ direction: "<=", bytes: 9 });
  });

  test("never prints the documented payload field", () => {
    const out = redactLogEvent({ type: "hw", message: "connected", data: { secret: "nope" } });
    expect(JSON.stringify(out)).not.toContain("nope");
    expect(out).toMatchObject({ type: "hw", message: "connected" });
  });

  test("keeps the message of non-APDU events", () => {
    const out = redactLogEvent({ type: "hw", message: "device disconnected", id: "7" });
    expect(out).toEqual({ type: "hw", message: "device disconnected", id: "7" });
  });

  test("tolerates an APDU event with no message", () => {
    expect(redactLogEvent({ type: "apdu" })).toMatchObject({ bytes: 0 });
  });
});
