import pytest

from exception import ClaNotSupportedError, InsNotSupportedError, WrongP1P2Error, \
    WrongDataLengthError, MetadatasParsingError, DeviceException

from passwordsManager_cmd import PasswordsManagerCommand


@pytest.mark.xfail(raises=ClaNotSupportedError)
def test_bad_cla(cmd: PasswordsManagerCommand):
    response = cmd.transport.exchange(cla=0xa0,  # 0xa0 instead of 0xe0
                                   ins=0x03,
                                   p1=0x00,
                                   p2=0x00,
                                   data=b"")

    raise DeviceException(error_code=response.status)


@pytest.mark.xfail(raises=InsNotSupportedError)
def test_bad_ins(cmd: PasswordsManagerCommand):
    response = cmd.transport.exchange(cla=0xe0,
                                   ins=0xAA,  # INS 0xAA is not supported
                                   p1=0x00,
                                   p2=0x00,
                                   data=b"")

    raise DeviceException(error_code=response.status)


@pytest.mark.xfail(raises=WrongP1P2Error)
def test_wrong_p1p2(cmd: PasswordsManagerCommand):
    response = cmd.transport.exchange(cla=0xe0,
                                   ins=0x03,
                                   p1=0x01,  # 0x01 instead of 0x00
                                   p2=0x00,
                                   data=b"")

    raise DeviceException(error_code=response.status)


@pytest.mark.xfail(raises=WrongDataLengthError)
def test_wrong_data_length(cmd: PasswordsManagerCommand):
    # APDUs must be at least 5 bytes: CLA, INS, P1, P2, Lc.
    response = cmd.transport.exchange_raw(bytes.fromhex("E000"))

    raise DeviceException(error_code=response.status)


@pytest.mark.xfail(raises=WrongDataLengthError)
def test_load_metadatas_with_too_much_data(cmd: PasswordsManagerCommand, test_vector):
    # [0] to avoid huge test names filled with the data.
    # Instead, it is filled with the data index
    metadatas = test_vector[0]
    cmd.load_metadatas(metadatas)


@pytest.mark.xfail(raises=MetadatasParsingError)
def test_load_metadatas_with_name_too_long(cmd: PasswordsManagerCommand, test_vector):
    # [0] to avoid huge test names filled with the data.
    # Instead, it is filled with the data index
    metadatas = test_vector[0]
    cmd.load_metadatas(metadatas)
