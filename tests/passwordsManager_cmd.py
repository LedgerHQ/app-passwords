import enum
import struct

from ledgercomm import Transport

from exception import DeviceException


CLA: int = 0xe0


class InsType(enum.IntEnum):
    INS_GET_VERSION = 0x03
    INS_GET_APP_NAME = 0x04
    INS_DUMP_METADATAS = 0x05
    INS_LOAD_METADATAS = 0x06
    INS_RUN_TEST = 0x99


class TestInsType(enum.IntEnum):
    INS_GENERATE_PASSWORD = 0x01


class PasswordsManagerCommand:
    def __init__(self,
                 transport: Transport,
                 debug: bool = False) -> None:
        self.transport = transport
        self.debug = debug

    def get_version(self) -> str:
        ins: InsType = InsType.INS_GET_VERSION

        self.transport.send(cla=CLA,
                            ins=ins,
                            p1=0x00,
                            p2=0x00,
                            payload=b"")

        sw, response = self.transport.recv()  # type: int, bytes

        if not sw & 0x9000:
            raise DeviceException(error_code=sw, ins=ins)

        assert len(response) == 3

        major, minor, patch = struct.unpack(
            "BBB",
            response
        )  # type: int, int, int

        return f"{major}.{minor}.{patch}"

    def get_app_name(self) -> str:
        ins: InsType = InsType.INS_GET_APP_NAME

        self.transport.send(cla=CLA,
                            ins=ins,
                            p1=0x00,
                            p2=0x00,
                            payload=b"")

        sw, response = self.transport.recv()  # type: int, bytes

        if not sw & 0x9000:
            raise DeviceException(error_code=sw, ins=ins)

        return response.decode("ascii")

    def generate_password(self, charsets: int, seed: str) -> str:
        assert charsets <= 0xFF
        assert len(seed) <= 20

        ins: InsType = InsType.INS_RUN_TEST
        testIns: TestInsType = TestInsType.INS_GENERATE_PASSWORD

        payload = charsets.to_bytes(
            1, "big") + bytes(seed, "utf-8")

        self.transport.send(cla=CLA,
                            ins=ins,
                            p1=testIns,
                            p2=0x00,
                            payload=payload)

        sw, response = self.transport.recv()  # type: int, bytes

        if not sw & 0x9000:
            raise DeviceException(error_code=sw, ins=ins)

        return response.decode("ascii")

    def dump_metadatas(self, size) -> bytes:
        ins: InsType = InsType.INS_DUMP_METADATAS

        metadatas = b""

        while len(metadatas) < size:
            self.transport.send(cla=CLA,
                                ins=ins,
                                p1=0x00,
                                p2=0x00,
                                payload=b"")

            sw, response = self.transport.recv()  # type: int, bytes

            if not sw & 0x9000:
                raise DeviceException(error_code=sw, ins=ins)

            metadatas += response[1:]

            if response[0] == 0xFF and len(metadatas) < size:
                raise Exception(
                    f"{size} bytes requested but only {len(metadatas)} bytes available")

        return metadatas[:size]

    def load_metadatas_chunk(self, chunk, is_last):
        ins: InsType = InsType.INS_LOAD_METADATAS

        self.transport.send(cla=CLA,
                            ins=ins,
                            p1=0xFF if is_last else 0x00,
                            p2=0x00,
                            payload=chunk)

        sw, response = self.transport.recv()  # type: int, bytes

        if not sw & 0x9000:
            raise DeviceException(error_code=sw, ins=ins)

    def load_metadatas(self, metadatas):

        chunks = [metadatas[i:i+255] for i in range(0, len(metadatas), 255)]

        for i, chunk in enumerate(chunks):
            is_last_chunk = True if i == len(chunks) else False
            self.load_metadatas_chunk(chunk, is_last_chunk)
