import TransportWebUSB from "@ledgerhq/hw-transport-webusb";

// Status word returned by the device when the user refuses the on-device
// Backup/Restore confirmation. Not a real failure, so the UI treats it apart.
export const SW_ACTION_CANCELLED = 0x6985;

// Mirrors MAX_METADATA_COUNT in src/types.h: MAX_METADATAS / (1 + 1 + 1 + MAX_METANAME).
const MAX_METADATA_COUNT = 178;

// Mirrors MAX_METANAME in src/types.h. An entry's length byte covers the charset byte plus the
// nickname, so it never exceeds this.
const MAX_METADATA_DATALEN = 20;

// Mirrors MAX_METADATAS in src/types.h: the largest metadata store the protocol can describe.
// The device reports its own size, but that value is untrusted, so it is bounded by this.
const MAX_METADATA_STORAGE_SIZE = 4096;

const insAPDU = Object.freeze({
  GET_APP_INFO_COMMAND: 0x01,
  GET_APP_CONFIG_COMMAND: 0x03,
  DUMP_METADATAS_COMMAND: 0x04,
  LOAD_METADATAS_COMMAND: 0x05,
});

const passwordsCharsets = Object.freeze({
  UPPERCASE: 1,
  LOWERCASE: 2,
  NUMBERS: 4,
  MINUS: 8,
  UNDERLINE: 16,
  SPACE: 32,
  SPECIAL: 64,
  BRACKETS: 128,
});

const allPasswordsCharsets = 0xff;

class PasswordsManager {
  constructor() {
    this.allowedStatuses = [
      0x9000,
      0x6985,
      0x6a86,
      0x6a87,
      0x6d00,
      0x6e00,
    ];
    this.connected = false;
    this.busy = false;
    this.transport = null;
    this.appName = null;
    this.version = null;
    this.storage_size = null;
    // Optional callback invoked when the device is unplugged mid-session.
    this.onDisconnect = null;
  }

  async connect() {
    if (!this.connected) {
      if (!this.transport) this.transport = await TransportWebUSB.create();
      // Surface physical unplugs to the UI.
      this.transport.on("disconnect", () => {
        this.connected = false;
        this.transport = null;
        if (this.onDisconnect) this.onDisconnect();
      });
      try {
        const [appName, version] = await this.getAppInfo();
        if (appName.toString() !== "Passwords")
          throw new Error("The Passwords app is not opened on the device");
        this.appName = appName;
        this.version = version;
        let appConfig = await this.getAppConfig();
        this.storage_size = appConfig["storage_size"];
        this.connected = true;
      } catch (error) {
        await this.disconnect();
        throw error;
      }
    }
  }

  isSuccess(result) {
    return (
      result.length >= 2 && result.readUInt16BE(result.length - 2) === 0x9000
    );
  }

  async disconnect() {
    if (this.transport) {
      try {
        await this.transport.close();
      } catch {
        // Ignore close errors (device may already be gone).
      }
    }
    this.connected = false;
    this.transport = null;
  }

  mapProtocolError(result) {
    if (result.length < 2) throw new Error("Response length is too small");

    var errors = {
      [SW_ACTION_CANCELLED]: "Action cancelled",
      0x6a86: "SW_WRONG_P1P2",
      0x6a87: "SW_WRONG_DATA_LENGTH",
      0x6d00: "SW_INS_NOT_SUPPORTED",
      0x6e00: "SW_CLA_NOT_SUPPORTED",
      0x6f10: "SW_METADATAS_PARSING_ERROR",
    };

    let error = result.readUInt16BE(result.length - 2);
    if (error in errors) {
      const err = new Error(errors[error]);
      // Expose the raw status word so callers can tell a user refusal apart
      // from an actual failure.
      err.statusWord = error;
      throw err;
    }
  }

  _lock() {
    if (this.busy) throw new Error("Device is busy");
    this.busy = true;
  }

  _unlock() {
    this.busy = false;
  }

  _charsetListToBitmask(charsets) {
    let bitmask = 0x00;
    for (const charset of charsets) {
      bitmask |= passwordsCharsets[charset];
    }
    if (bitmask === 0x00) bitmask = allPasswordsCharsets;
    return bitmask;
  }

  _bitmaskToCharsetList(bitmask) {
    let charsetList = [];
    if (bitmask === 0x00 || bitmask === allPasswordsCharsets) {
      charsetList.push("ALL_SETS");
    } else {
      for (const charset in passwordsCharsets) {
        if (passwordsCharsets[charset] & bitmask) charsetList.push(charset);
      }
    }
    return charsetList;
  }

  /* The storage size comes from the device. getAppConfig() validates it before it is stored,
   * but the allocation and loop bounds re-check it here so they can never be driven by a value
   * that reached the instance some other way. */
  _validatedStorageSize() {
    const size = this.storage_size;
    if (
      !Number.isSafeInteger(size) ||
      size <= 0 ||
      size > MAX_METADATA_STORAGE_SIZE
    )
      throw new Error(`Unexpected metadata storage size: ${size}`);
    return size;
  }

  _toBytes(json_metadatas) {
    const storage_size = this._validatedStorageSize();
    let metadatas = Buffer.alloc(storage_size);
    let parsed_metadatas = JSON.parse(json_metadatas)["parsed"];
    let offset = 0;
    // The device list arrays are sized for MAX_METADATA_COUNT entries; reject a backup that
    // would exceed that here too, rather than relying on the device to refuse it.
    if (parsed_metadatas.length > MAX_METADATA_COUNT)
      throw new Error(
        `Too many entries in this backup (${MAX_METADATA_COUNT} max): ${parsed_metadatas.length}`
      );
    parsed_metadatas.forEach((element) => {
      const nickname = element["nickname"];
      const charsets = this._charsetListToBitmask(element["charsets"]);
      if (typeof nickname !== "string" || nickname.length === 0)
        throw new Error("This backup contains an entry with an empty nickname");
      // The device keyboard can only produce printable ASCII, and the nickname is what the
      // password is derived from. Anything else would be stored as bytes the device cannot
      // display and cannot be retyped, so refuse it rather than write it.
      if (!/^[\x20-\x7e]+$/.test(nickname))
        throw new Error(
          `Nickname must be printable ASCII only: ${JSON.stringify(nickname)}`
        );
      // The length byte and the offset are byte counts. String.length counts UTF-16 code
      // units, so a non-ASCII nickname used to record fewer bytes than Buffer.write() emitted:
      // the entry claimed the wrong length and the UTF-8 expansion overwrote the next record's
      // header. The ASCII check above makes the two equal, and using byteLength keeps them so.
      const nicknameBytes = Buffer.byteLength(nickname, "utf8");
      if (nicknameBytes > MAX_METADATA_DATALEN - 1)
        throw new Error(
          `Nickname too long (${MAX_METADATA_DATALEN - 1} bytes max): ${nickname} is ${nicknameBytes} bytes`
        );
      if (offset + 3 + nicknameBytes >= storage_size)
        throw new Error(
          `Not enough memory on this device to restore this backup`
        );
      metadatas[offset++] = nicknameBytes + 1;
      metadatas[offset++] = 0x00;
      metadatas[offset++] = charsets;
      metadatas.write(nickname, offset, nicknameBytes, "utf8");
      offset += nicknameBytes;
    });
    // mark free space at the end of the buffer
    metadatas[offset++] = 0x00;
    metadatas[offset++] = 0x00;
    return metadatas;
  }

  _toJSON(metadatas) {
    const metadatas_list = [];
    const erased_list = [];
    // Kept for backup-file compatibility. Malformed data now aborts the parse instead of
    // being recorded and walked past, so this stays empty.
    const corruptions = [];
    let offset = 0;
    let terminated = false;

    // The device response is untrusted: a corrupted dump, or a device that merely claims to be
    // the Passwords app, must not be able to walk this loop past the end of the buffer. Reading
    // past the end yields `undefined`, which turned `offset` into NaN and spun forever, hanging
    // the tab.
    while (offset < metadatas.length) {
      const len = metadatas[offset];
      if (len === 0) {
        terminated = true;
        break;
      }
      if (len > MAX_METADATA_DATALEN || offset + 2 + len > metadatas.length) {
        throw new Error(
          `Malformed metadata at offset ${offset}: entry length ${len}`
        );
      }
      if (metadatas_list.length + erased_list.length >= MAX_METADATA_COUNT) {
        throw new Error(
          `Malformed metadata: more than ${MAX_METADATA_COUNT} entries`
        );
      }
      const erased = metadatas[offset + 1] === 0xff;
      const charsets = metadatas[offset + 2];
      const metadata = {
        nickname: metadatas.slice(offset + 3, offset + 2 + len).toString(),
        charsets: this._bitmaskToCharsetList(charsets),
      };
      (erased ? erased_list : metadatas_list).push(metadata);
      offset += len + 2;
    }

    if (!terminated) {
      throw new Error("Malformed metadata: missing terminator");
    }

    return {
      parsed: metadatas_list,
      nicknames_erased_but_still_stored: erased_list,
      corruptions_encountered: corruptions,
      raw_metadatas: metadatas.toString("hex"),
    };
  }

  async _load_metadatas_chunk(chunk, is_last) {
    let result = await this.transport.send(
      0xe0,
      insAPDU.LOAD_METADATAS_COMMAND,
      is_last ? 0xff : 0x00,
      0x00,
      Buffer.from(chunk),
      this.allowedStatuses
    );
    if (!this.isSuccess(result)) this.mapProtocolError(result);
    return result;
  }
  async getAppInfo() {
    this._lock();
    try {
      let result = await this.transport.send(
        0xb0,
        insAPDU.GET_APP_INFO_COMMAND,
        0x00,
        0x00,
        Buffer.alloc(0),
        this.allowedStatuses
      );
      if (!this.isSuccess(result)) this.mapProtocolError(result);

      result = result.slice(0, result.length - 2);
      let app_name, app_version;
      try {
        let offset = 1;
        let app_name_length = result[offset++];
        app_name = result.slice(offset, offset + app_name_length).toString();
        offset += app_name_length;
        let app_version_length = result[offset++];
        app_version = result
          .slice(offset, offset + app_version_length)
          .toString();
        return [app_name, app_version];
      } catch (error) {
        throw new Error(
          `Unexpected result from device, parsing error: ${error}`
        );
      }
    } finally {
      this._unlock();
    }
  }

  async getAppConfig() {
    this._lock();
    try {
      let result = await this.transport.send(
        0xe0,
        insAPDU.GET_APP_CONFIG_COMMAND,
        0x00,
        0x00,
        Buffer.alloc(0),
        this.allowedStatuses
      );
      if (!this.isSuccess(result)) this.mapProtocolError(result);
      result = result.slice(0, result.length - 2);
      if (result.length !== 6)
        throw new Error(`Can't parse app config of length ${result.length}`);

      // The size is used as an allocation size and a loop bound, so it is validated before it
      // leaves this method. A device that only claims to be the Passwords app could otherwise
      // report 0xffffffff and have the page allocate 4 GiB.
      const storage_size = result.readUInt32BE(0);
      if (
        !Number.isSafeInteger(storage_size) ||
        storage_size <= 0 ||
        storage_size > MAX_METADATA_STORAGE_SIZE
      )
        throw new Error(`Unexpected metadata storage size: ${storage_size}`);

      const keyboard_type = result[4];
      const press_enter_after_typing = result[5];
      return { storage_size, keyboard_type, press_enter_after_typing };
    } finally {
      this._unlock();
    }
  }

  async dump_metadatas() {
    this._lock();
    try {
      const total = this._validatedStorageSize();
      let metadatas = Buffer.alloc(0);
      while (metadatas.length < total) {
        let result = await this.transport.send(
          0xe0,
          insAPDU.DUMP_METADATAS_COMMAND,
          0x00,
          0x00,
          Buffer.alloc(0),
          this.allowedStatuses
        );
        if (!this.isSuccess(result)) this.mapProtocolError(result);
        const chunk = Buffer.from(result.slice(1, -2));
        // Every response has to make forward progress without overshooting the total: an empty
        // chunk would keep this loop (and the tab) spinning forever, and the total is what
        // bounds how much memory the dump can take. The chunk size itself is deliberately not
        // capped here -- it is MAX_PAYLOAD_SIZE on the device, derived from the SDK's APDU
        // buffer, so it varies by target and is not the client's business.
        if (chunk.length === 0 || metadatas.length + chunk.length > total)
          throw new Error(
            `Invalid metadata dump chunk of ${chunk.length} bytes from the device`
          );
        metadatas = Buffer.concat([metadatas, chunk]);
        if (result[0] === 0xff && metadatas.length < total) {
          throw new Error(
            `${total} bytes requested but only ${metadatas.length} bytes available`
          );
        }
      }
      return this._toJSON(metadatas);
    } finally {
      this._unlock();
    }
  }

  async load_metadatas(JSON_metadatas) {
    this._lock();
    try {
      let metadatas = this._toBytes(JSON_metadatas);
      if (metadatas.length === 0) {
        throw new Error("No data to load");
      }
      for (let i = 0; i < metadatas.length; i += 0xff) {
        let chunk = metadatas.slice(i, i + 0xff);
        await this._load_metadatas_chunk(
          chunk,
          i + chunk.length === metadatas.length ? true : false
        );
      }
    } finally {
      this._unlock();
    }
  }
}

export default PasswordsManager;
