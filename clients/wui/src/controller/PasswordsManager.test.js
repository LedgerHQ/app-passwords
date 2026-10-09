import PasswordsManager, { SW_ACTION_CANCELLED } from "./PasswordsManager.js";

function makeManager() {
  const m = new PasswordsManager();
  m.storage_size = 4096;
  m.generation_version = 1;
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

  test("ALL_SETS round-trips back to the full mask", () => {
    // What _bitmaskToCharsetList() writes into a backup has to be readable again.
    expect(m._charsetListToBitmask(["ALL_SETS"])).toBe(0xff);
  });

  test("rejects an unknown charset name instead of silently dropping it", () => {
    // A typo used to OR in `undefined`, i.e. nothing, changing the derived password.
    expect(() => m._charsetListToBitmask(["UPPERCASE", "NUMBER"])).toThrow(/unknown charset/);
  });

  test("rejects a list of nothing but unknown names", () => {
    // These used to leave the mask at 0 and fall through to the ALL_SETS default.
    expect(() => m._charsetListToBitmask(["ALPHA", "BETA"])).toThrow(/unknown charset/);
  });

  test("rejects an inherited property name", () => {
    // `"constructor" in passwordsCharsets` is true, and its value ORs in as 0.
    expect(() => m._charsetListToBitmask(["constructor"])).toThrow(/unknown charset/);
    expect(() => m._charsetListToBitmask(["toString"])).toThrow(/unknown charset/);
  });

  test("rejects a charsets field that is not a list", () => {
    expect(() => m._charsetListToBitmask("UPPERCASE")).toThrow(/not a list/);
    expect(() => m._charsetListToBitmask(undefined)).toThrow(/not a list/);
    expect(() => m._charsetListToBitmask({ UPPERCASE: true })).toThrow(/not a list/);
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
      { nickname: "github", charsets: ["UPPERCASE", "LOWERCASE", "NUMBERS"], version: 0 },
      // An empty charset list is stored as "all", so it decodes to ALL_SETS.
      { nickname: "email", charsets: ["ALL_SETS"], version: 0 },
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

  test("rejects a non-ASCII nickname instead of misreporting its length", () => {
    // "é" is one UTF-16 code unit but two UTF-8 bytes. The length byte and the offset used to
    // be taken from String.length, so the entry announced 2 bytes while 3 were written and the
    // next record's header was overwritten.
    const input = { parsed: [{ nickname: "café", charsets: [] }] };
    expect(() => m._toBytes(JSON.stringify(input))).toThrow(
      /printable ASCII/i
    );
  });

  test("rejects a nickname with a control character", () => {
    const input = { parsed: [{ nickname: "ma\nil", charsets: [] }] };
    expect(() => m._toBytes(JSON.stringify(input))).toThrow(
      /printable ASCII/i
    );
  });

  test("a 19-byte ASCII nickname still fits, 20 does not", () => {
    const ok = { parsed: [{ nickname: "x".repeat(19), charsets: [] }] };
    expect(() => m._toBytes(JSON.stringify(ok))).not.toThrow();
    const tooLong = { parsed: [{ nickname: "x".repeat(20), charsets: [] }] };
    expect(() => m._toBytes(JSON.stringify(tooLong))).toThrow(/too long/i);
  });

  test("entries after a multi-byte-looking one keep their own headers", () => {
    // Round-trip two entries: with the old byte accounting the second record's header was
    // clobbered by the first nickname's UTF-8 expansion.
    const input = {
      parsed: [
        { nickname: "first", charsets: [] },
        { nickname: "second", charsets: [] },
      ],
    };
    const out = m._toJSON(m._toBytes(JSON.stringify(input)));
    expect(out.parsed.map((e) => e.nickname)).toEqual(["first", "second"]);
  });
});

describe("device-reported storage size is untrusted", () => {
  // getAppConfig() parses this 6-byte payload: storage size (4), keyboard type, press-enter.
  const appConfig = (size) => {
    const buf = Buffer.alloc(8);
    buf.writeUInt32BE(size, 0);
    buf.writeUInt16BE(0x9000, 6);
    return buf;
  };

  const managerAnswering = (payload) => {
    const m = new PasswordsManager();
    m.transport = { send: async () => payload };
    return m;
  };

  test("rejects an oversized storage size instead of allocating it", async () => {
    // 0xffffffff used to reach Buffer.alloc() and the dump loop bound directly.
    await expect(managerAnswering(appConfig(0xffffffff)).getAppConfig()).rejects.toThrow(
      /Unexpected metadata storage size/
    );
  });

  test("rejects a zero storage size", async () => {
    await expect(managerAnswering(appConfig(0)).getAppConfig()).rejects.toThrow(
      /Unexpected metadata storage size/
    );
  });

  test("accepts the size the firmware actually reports", async () => {
    const config = await managerAnswering(appConfig(4096)).getAppConfig();
    expect(config.storage_size).toBe(4096);
  });

  test("allocation refuses a size that reached the instance unvalidated", () => {
    const m = new PasswordsManager();
    m.storage_size = 0xffffffff;
    const input = { parsed: [{ nickname: "mail", charsets: [] }] };
    expect(() => m._toBytes(JSON.stringify(input))).toThrow(
      /Unexpected metadata storage size/
    );
  });
});

describe("dump_metadatas drives the transfer to completion", () => {
  // The device answers MAX_PAYLOAD_SIZE bytes per chunk, which is derived from the SDK's APDU
  // buffer and is larger than 255 -- 270 on Flex. A client-side cap on the chunk size broke
  // every real backup, so a full dump is exercised at that size.
  const DEVICE_CHUNK = 270;

  const managerDumping = (chunkSizes) => {
    const m = new PasswordsManager();
    m.storage_size = 4096;
    let sent = 0;
    let call = 0;
    m.transport = {
      send: async () => {
        const size = chunkSizes[Math.min(call++, chunkSizes.length - 1)];
        const remaining = 4096 - sent;
        const payload = Buffer.alloc(Math.min(size, remaining));
        sent += payload.length;
        return Buffer.concat([
          Buffer.from([sent >= 4096 ? 0xff : 0x00]),
          payload,
          Buffer.from([0x90, 0x00]),
        ]);
      },
    };
    return m;
  };

  test("accepts the chunk size the firmware actually sends", async () => {
    const out = await managerDumping([DEVICE_CHUNK]).dump_metadatas();
    // An all-zero store parses as an empty database.
    expect(out.parsed).toEqual([]);
    expect(out.raw_metadatas).toHaveLength(4096 * 2);
  });

  test("rejects a device that stops making progress", async () => {
    await expect(managerDumping([0]).dump_metadatas()).rejects.toThrow(
      /Invalid metadata dump chunk of 0 bytes/
    );
  });

  test("rejects a chunk that would overshoot the announced total", async () => {
    const m = new PasswordsManager();
    m.storage_size = 4096;
    m.transport = {
      send: async () =>
        Buffer.concat([
          Buffer.from([0x00]),
          Buffer.alloc(5000),
          Buffer.from([0x90, 0x00]),
        ]),
    };
    await expect(m.dump_metadatas()).rejects.toThrow(
      /Invalid metadata dump chunk of 5000 bytes/
    );
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
      { nickname: "mail", charsets: ["UPPERCASE", "LOWERCASE", "NUMBERS"], version: 0 },
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

describe("generation versions", () => {
  const backup = (entries) => JSON.stringify({ parsed: entries });
  // Kind byte of the first entry of a serialized backup.
  const firstKind = (bytes) => bytes[1];

  test("an entry without a version field is version 0", () => {
    const bytes = makeManager()._toBytes(backup([{ nickname: "legacy", charsets: [] }]));
    expect(firstKind(bytes)).toBe(0x00);
  });

  test("a version 1 entry round-trips", () => {
    const m = makeManager();
    const bytes = m._toBytes(backup([{ nickname: "new", charsets: ["ALL_SETS"], version: 1 }]));
    expect(firstKind(bytes)).toBe(0x01);
    // A version 1 entry without a length field is 20 characters long.
    expect(m._toJSON(bytes).parsed).toEqual([
      { nickname: "new", charsets: ["ALL_SETS"], version: 1, length: 20 },
    ]);
  });

  test("a backup of an old app restores as version 0 on a new app", () => {
    // An old app dump: every live entry has kind 0x00.
    const dump = Buffer.concat([
      Buffer.from([0x05, 0x00, 0x07]),
      Buffer.from("mail", "ascii"),
      Buffer.alloc(10),
    ]);
    const m = makeManager();
    const restored = m._toBytes(JSON.stringify(m._toJSON(dump)));
    expect(firstKind(restored)).toBe(0x00);
  });

  test("refuses to restore a version 1 entry on an app that only knows version 0", () => {
    const m = makeManager();
    m.generation_version = 0;
    expect(() =>
      m._toBytes(backup([{ nickname: "new", charsets: [], version: 1 }]))
    ).toThrow(/needs version 1\.4\.0 or later of the Passwords app/);
    // A version 0 backup still restores there.
    expect(() => m._toBytes(backup([{ nickname: "old", charsets: [] }]))).not.toThrow();
  });

  test("refuses an unknown or malformed version", () => {
    const m = makeManager();
    for (const version of [2, -1, 1.5, "1", null]) {
      expect(() =>
        m._toBytes(backup([{ nickname: "x", charsets: [], version }]))
      ).toThrow(/unknown version/);
    }
  });

  test("refuses a dump holding an entry of an unknown version", () => {
    const dump = Buffer.concat([
      Buffer.from([0x02, 0x02, 0x07]),
      Buffer.from("x", "ascii"),
      Buffer.alloc(10),
    ]);
    expect(() => makeManager()._toJSON(dump)).toThrow(/Unknown entry version 2/);
  });

  const appConfigAnswer = (payload) => {
    const m = new PasswordsManager();
    m.transport = { send: async () => Buffer.concat([payload, Buffer.from([0x90, 0x00])]) };
    return m;
  };

  test("an app config without the version byte comes from a version 0 app", async () => {
    const config = await appConfigAnswer(Buffer.from([0, 0, 0x10, 0, 1, 0])).getAppConfig();
    expect(config.generation_version).toBe(0);
  });

  test("reads the generation version from the app config", async () => {
    const config = await appConfigAnswer(Buffer.from([0, 0, 0x10, 0, 1, 0, 1])).getAppConfig();
    expect(config.generation_version).toBe(1);
  });
});

describe("password lengths", () => {
  const backup = (entries) => JSON.stringify({ parsed: entries });

  test("a version 1 entry stores its length before the nickname", () => {
    const m = makeManager();
    const bytes = m._toBytes(
      backup([{ nickname: "new", charsets: ["ALL_SETS"], version: 1, length: 64 }])
    );
    expect([...bytes.subarray(0, 7)]).toEqual([5, 0x01, 0xff, 64, ...Buffer.from("new")]);
    expect(m._toJSON(bytes).parsed).toEqual([
      { nickname: "new", charsets: ["ALL_SETS"], version: 1, length: 64 },
    ]);
  });

  test("a version 0 entry keeps its layout and has no length field", () => {
    const m = makeManager();
    const bytes = m._toBytes(backup([{ nickname: "old", charsets: [] }]));
    expect([...bytes.subarray(0, 6)]).toEqual([4, 0x00, 0xff, ...Buffer.from("old")]);
    expect(m._toJSON(bytes).parsed).toEqual([{ nickname: "old", charsets: ["ALL_SETS"], version: 0 }]);
  });

  test("a version 1 entry fits the longest nickname", () => {
    const m = makeManager();
    const nickname = "a".repeat(19);
    const bytes = m._toBytes(backup([{ nickname, charsets: [], version: 1, length: 8 }]));
    expect(bytes[0]).toBe(21);
    expect(m._toJSON(bytes).parsed[0]).toMatchObject({ nickname, length: 8 });
  });

  test("refuses unsupported lengths", () => {
    const m = makeManager();
    for (const length of [0, 13, 65, "20", null]) {
      expect(() =>
        m._toBytes(backup([{ nickname: "x", charsets: [], version: 1, length }]))
      ).toThrow(/unsupported length/);
    }
  });

  test("refuses a length other than 20 on a version 0 entry", () => {
    const m = makeManager();
    expect(() => m._toBytes(backup([{ nickname: "x", charsets: [], length: 32 }]))).toThrow(
      /always 20 characters/
    );
    expect(() => m._toBytes(backup([{ nickname: "x", charsets: [], length: 20 }]))).not.toThrow();
  });

  test("refuses a dump holding an unsupported length", () => {
    const dump = Buffer.concat([Buffer.from([0x03, 0x01, 0x07, 13]), Buffer.from("x"), Buffer.alloc(10)]);
    expect(() => makeManager()._toJSON(dump)).toThrow(/Unsupported password length 13/);
  });

  test("an erased entry of the longest layout still parses", () => {
    const dump = Buffer.concat([Buffer.from([21, 0xff]), Buffer.alloc(21), Buffer.alloc(10)]);
    const out = makeManager()._toJSON(dump);
    expect(out.parsed).toEqual([]);
    expect(out.nicknames_erased_but_still_stored).toHaveLength(1);
  });
});

describe("weak charsets", () => {
  const backup = (entries) => JSON.stringify({ parsed: entries });

  test("refuses a new entry using separators only", () => {
    for (const charsets of [["MINUS"], ["SPACE"], ["MINUS", "UNDERLINE", "SPACE"]]) {
      expect(() =>
        makeManager()._toBytes(backup([{ nickname: "x", charsets, version: 1 }]))
      ).toThrow(/separators only/);
    }
  });

  test("restores a version 0 entry using separators only", () => {
    const bytes = makeManager()._toBytes(backup([{ nickname: "old", charsets: ["MINUS"] }]));
    expect([...bytes.subarray(0, 3)]).toEqual([4, 0x00, 0x08]);
  });

  test("lists the entries using separators only", () => {
    const parsed = [
      { nickname: "weak", charsets: ["MINUS", "SPACE"] },
      { nickname: "strong", charsets: ["MINUS", "NUMBERS"] },
      { nickname: "all", charsets: ["ALL_SETS"] },
    ];
    expect(makeManager().weakNicknames(parsed)).toEqual(["weak"]);
  });

  test("load_metadatas reports the weak entries it restored", async () => {
    const m = makeManager();
    m.transport = { send: async () => Buffer.from([0x90, 0x00]) };
    const weak = await m.load_metadatas(
      backup([
        { nickname: "weak", charsets: ["UNDERLINE"] },
        { nickname: "fine", charsets: [] },
      ])
    );
    expect(weak).toEqual(["weak"]);
  });
});
