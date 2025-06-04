import enum

from exception import DeviceException
from ledgered.devices import Device
from ragger.backend import BackendInterface
from ragger.firmware.touch.positions import STAX_BUTTON_ABOVE_LOWER_MIDDLE

CLA_SDK: int = 0xb0
CLA: int = 0xe0


class InsType(enum.IntEnum):
    INS_GET_APP_INFO = 0x01
    INS_GET_APP_CONFIG = 0x03
    INS_DUMP_METADATAS = 0x04
    INS_LOAD_METADATAS = 0x05
    INS_RUN_TEST = 0x99


class TestInsType(enum.IntEnum):
    INS_GENERATE_PASSWORD = 0x01


class PasswordsManagerCommand:
    def __init__(self,
                 transport: BackendInterface,
                 device: Device,
                 debug: bool = False) -> None:
        self.transport = transport
        self.device = device
        self.debug = debug
        self.approved: bool = False

    def approve(self):
        if self.device.touchable:
            self.transport.finger_touch(*STAX_BUTTON_ABOVE_LOWER_MIDDLE)
        else:
            self.transport.both_click()

    def get_app_info(self) -> str:
        ins: InsType = InsType.INS_GET_APP_INFO

        response = self.transport.exchange(cla=CLA_SDK, ins=ins)
        sw, response = response.status, response.data

        if not sw & 0x9000:
            raise DeviceException(error_code=sw, ins=ins)

        offset = 1
        app_name_length = response[offset]
        offset += 1
        app_name = response[offset:offset+app_name_length].decode("ascii")
        offset += app_name_length
        app_version_length = response[offset]
        offset += 1
        app_version = response[offset:offset +
                               app_version_length].decode("ascii")

        return app_name, app_version

    def get_app_config(self) -> str:
        ins: InsType = InsType.INS_GET_APP_CONFIG

        response =self.transport.exchange(cla=CLA, ins=ins)
        sw, response = response.status, response.data

        if not sw & 0x9000:
            raise DeviceException(error_code=sw, ins=ins)

        assert len(response) == 6

        storage_size = int.from_bytes(response[:4], "big")
        keyboard_type = response[4]
        press_enter_after_typing = response[5]

        return storage_size, keyboard_type, press_enter_after_typing

    def reset_approval_state(self):
        # dummy call just to reset internal approval state
        self.get_app_config()

    def generate_password(self, charsets: int, seed: str) -> str:
        assert charsets <= 0xFF
        assert len(seed) <= 20

        ins: InsType = InsType.INS_RUN_TEST
        testIns: TestInsType = TestInsType.INS_GENERATE_PASSWORD

        payload = charsets.to_bytes(
            1, "big") + bytes(seed, "utf-8")

        response = self.transport.exchange(cla=CLA,
                                           ins=ins,
                                           p1=testIns,
                                           data=payload)
        sw, response = response.status, response.data

        if not sw & 0x9000:
            raise DeviceException(error_code=sw, ins=ins)

        return response.decode("ascii")

    def dump_metadatas(self, size) -> bytes:
        ins: InsType = InsType.INS_DUMP_METADATAS

        metadatas = b""
        self.approved = False
        while len(metadatas) < size:
            if not self.approved:
                with self.transport.exchange_async(cla=CLA, ins=ins):
                    self.approve()
                response = self.transport.last_async_response
                self.approved = True
            else:
                response = self.transport.exchange(cla=CLA, ins=ins)
            sw, response = response.status, response.data
            if not sw & 0x9000:
                raise DeviceException(error_code=sw, ins=ins)

            metadatas += response[1:]

            if response[0] == 0xFF and len(metadatas) < size:
                raise Exception(
                    f"{size} bytes requested but only {len(metadatas)} bytes available")

        return metadatas[:size]

    def load_metadatas_chunk(self, chunk, is_last):
        ins: InsType = InsType.INS_LOAD_METADATAS
        if not self.approved:
            with self.transport.exchange_async(cla=CLA,
                                               ins=ins,
                                               p1=0xFF if is_last else 0x00,
                                               data=chunk):
                self.approve()
            response = self.transport.last_async_response
            self.approved = True
        else:
            response = self.transport.exchange(cla=CLA,
                                               ins=ins,
                                               p1=0xFF if is_last else 0x00,
                                               data=chunk)
        sw, response = response.status, response.data

        if not sw & 0x9000:
            raise DeviceException(error_code=sw, ins=ins)

    def load_metadatas(self, metadatas):

        chunks = [metadatas[i:i+255] for i in range(0, len(metadatas), 255)]

        self.approved = False
        for i, chunk in enumerate(chunks):
            is_last_chunk = True if i+1 == len(chunks) else False
            self.load_metadatas_chunk(chunk, is_last_chunk)
