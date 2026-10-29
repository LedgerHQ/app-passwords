import { Buffer } from "buffer";

// The Ledger transport and the APDU layer use the Node Buffer API as a global.
globalThis.Buffer = Buffer;
