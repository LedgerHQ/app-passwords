import enum
from typing import Any, ClassVar

from .types import (
    ActionCancelledError,
    ClaNotSupportedError,
    InsNotSupportedError,
    MetadatasParsingError,
    UnknownDeviceError,
    WrongDataLengthError,
    WrongP1P2Error,
)


class DeviceException(Exception):  # pylint: disable=too-few-public-methods
    exc: ClassVar[dict[int, Any]] = {
        0x6A86: WrongP1P2Error,
        0x6A87: WrongDataLengthError,
        0x6D00: InsNotSupportedError,
        0x6E00: ClaNotSupportedError,
        0x6985: ActionCancelledError,
        0x6F10: MetadatasParsingError,
    }

    def __new__(cls, error_code: int, ins: enum.IntEnum | None = None, message: str = "") -> Any:
        error_message: str = f"Error in {ins!r} command" if ins else "Error in command"

        if error_code in DeviceException.exc:
            return DeviceException.exc[error_code](hex(error_code), error_message, message)

        return UnknownDeviceError(hex(error_code), error_message, message)
