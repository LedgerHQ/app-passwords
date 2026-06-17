import PasswordsManager from "./PasswordsManager.js";

function makeManager() {
  const m = new PasswordsManager();
  m.storage_size = 4096;
  return m;
}

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
});
