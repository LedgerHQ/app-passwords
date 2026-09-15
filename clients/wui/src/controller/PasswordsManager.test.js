import PasswordsManager, { SW_ACTION_CANCELLED } from "./PasswordsManager.js";

function makeManager() {
  const m = new PasswordsManager();
  m.storage_size = 4096;
  return m;
}

describe("protocol error mapping", () => {
  const m = makeManager();

  test("a user refusal throws with the cancellation status word", () => {
    const refused = Buffer.from([0x69, 0x85]);
    expect(() => m.mapProtocolError(refused)).toThrow(/Action cancelled/i);
    try {
      m.mapProtocolError(refused);
    } catch (error) {
      expect(error.statusWord).toBe(SW_ACTION_CANCELLED);
    }
  });

  test("other known errors carry their own status word", () => {
    try {
      m.mapProtocolError(Buffer.from([0x6a, 0x86]));
    } catch (error) {
      expect(error.statusWord).toBe(0x6a86);
    }
  });
});

describe("charset bitmask mapping", () => {
  const m = makeManager();

  test("maps a list of charsets to a bitmask", () => {
    // UPPERCASE (1) | NUMBERS (4) = 5
    expect(m._charsetListToBitmask(["UPPERCASE", "NUMBERS"])).toBe(0x05);
  });

  test("an empty list means all charsets (0xff)", () => {
    expect(m._charsetListToBitmask([])).toBe(0xff);
  });

  test("decodes a bitmask back to its charset names", () => {
    expect(m._bitmaskToCharsetList(0x05)).toEqual(["UPPERCASE", "NUMBERS"]);
  });

  test("0xff and 0x00 both decode to ALL_SETS", () => {
    expect(m._bitmaskToCharsetList(0xff)).toEqual(["ALL_SETS"]);
    expect(m._bitmaskToCharsetList(0x00)).toEqual(["ALL_SETS"]);
  });
});

describe("metadata serialization round-trip", () => {
  const m = makeManager();

  test("_toBytes then _toJSON preserves the entries", () => {
    const input = {
      parsed: [
        { nickname: "github", charsets: ["UPPERCASE", "LOWERCASE", "NUMBERS"] },
        { nickname: "email", charsets: [] },
      ],
    };

    const bytes = m._toBytes(JSON.stringify(input));
    const out = m._toJSON(bytes);

    expect(out.parsed).toEqual([
      { nickname: "github", charsets: ["UPPERCASE", "LOWERCASE", "NUMBERS"] },
      // An empty charset list is stored as "all", so it decodes to ALL_SETS.
      { nickname: "email", charsets: ["ALL_SETS"] },
    ]);
    expect(out.nicknames_erased_but_still_stored).toEqual([]);
  });

  test("rejects a nickname longer than 19 characters", () => {
    const input = { parsed: [{ nickname: "x".repeat(20), charsets: [] }] };
    expect(() => m._toBytes(JSON.stringify(input))).toThrow(/too long/i);
  });

  test("rejects a backup that does not fit in device storage", () => {
    const tiny = new PasswordsManager();
    tiny.storage_size = 8;
    const input = {
      parsed: [
        { nickname: "one", charsets: [] },
        { nickname: "two", charsets: [] },
      ],
    };
    expect(() => tiny._toBytes(JSON.stringify(input))).toThrow(/Not enough memory/i);
  });

  test("rejects a backup holding more entries than the device can list", () => {
    // The device list arrays hold MAX_METADATA_COUNT (178) entries, which is fewer than the
    // number of short entries that fit in the 4096-byte store.
    const input = {
      parsed: Array.from({ length: 179 }, (_, i) => ({
        nickname: `n${i}`,
        charsets: [],
      })),
    };
    expect(() => m._toBytes(JSON.stringify(input))).toThrow(/too many entries/i);
  });

  test("accepts a backup with exactly the maximum number of entries", () => {
    const input = {
      parsed: Array.from({ length: 178 }, (_, i) => ({
        nickname: `n${i}`,
        charsets: [],
      })),
    };
    expect(() => m._toBytes(JSON.stringify(input))).not.toThrow();
  });

  test("rejects an entry with an empty nickname", () => {
    const input = { parsed: [{ nickname: "", charsets: [] }] };
    expect(() => m._toBytes(JSON.stringify(input))).toThrow(/empty nickname/i);
  });
});

describe("metadata parsing treats the device response as untrusted", () => {
  const m = makeManager();

  // One live entry: length byte (charset + nickname), kind, charset, nickname.
  const entry = (nickname) =>
    Buffer.concat([
      Buffer.from([nickname.length + 1, 0x00, 0x07]),
      Buffer.from(nickname, "ascii"),
    ]);

  test("stops at the terminator and ignores the trailing slack", () => {
    const buf = Buffer.concat([entry("mail"), Buffer.alloc(100)]);
    const out = m._toJSON(buf);
    expect(out.parsed).toEqual([
      { nickname: "mail", charsets: ["UPPERCASE", "LOWERCASE", "NUMBERS"] },
    ]);
  });

  test("throws instead of hanging when no terminator is present", () => {
    // Records tiling the whole buffer: the old loop read past the end, `len` became undefined,
    // `offset` became NaN, the terminator was never reached and the tab froze.
    const buf = Buffer.concat(Array.from({ length: 20 }, () => entry("ab")));
    expect(() => m._toJSON(buf)).toThrow(/missing terminator/i);
  });

  test("throws when a record runs past the end of the buffer", () => {
    // Announces 20 payload bytes with only a few left.
    const buf = Buffer.from([0x14, 0x00, 0x07, 0x61, 0x62]);
    expect(() => m._toJSON(buf)).toThrow(/Malformed metadata at offset 0/);
  });

  test("throws on an entry longer than the device maximum", () => {
    const buf = Buffer.concat([
      Buffer.from([21, 0x00, 0x07]),
      Buffer.alloc(21, 0x61),
      Buffer.alloc(10),
    ]);
    expect(() => m._toJSON(buf)).toThrow(/entry length 21/);
  });

  test("throws when the dump holds more entries than the device can list", () => {
    const buf = Buffer.concat([
      ...Array.from({ length: 179 }, () => entry("a")),
      Buffer.alloc(2),
    ]);
    expect(() => m._toJSON(buf)).toThrow(/more than 178 entries/i);
  });

  test("an empty buffer is a missing terminator, not an empty database", () => {
    expect(() => m._toJSON(Buffer.alloc(0))).toThrow(/missing terminator/i);
  });

  test("a zero-filled buffer parses as an empty database", () => {
    const out = m._toJSON(Buffer.alloc(4096));
    expect(out.parsed).toEqual([]);
    expect(out.nicknames_erased_but_still_stored).toEqual([]);
  });
});
