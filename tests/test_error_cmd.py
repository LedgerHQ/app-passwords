import pytest

from exception import *


@pytest.mark.xfail(raises=ClaNotSupportedError)
def test_bad_cla(cmd):
    sw, _ = cmd.transport.exchange(cla=0xa0,  # 0xa0 instead of 0xe0
                                   ins=0x03,
                                   p1=0x00,
                                   p2=0x00,
                                   cdata=b"")

    raise DeviceException(error_code=sw)


@pytest.mark.xfail(raises=InsNotSupportedError)
def test_bad_ins(cmd):
    sw, _ = cmd.transport.exchange(cla=0xe0,
                                   ins=0xAA,  # INS 0xAA is not supported
                                   p1=0x00,
                                   p2=0x00,
                                   cdata=b"")

    raise DeviceException(error_code=sw)


@pytest.mark.xfail(raises=WrongP1P2Error)
def test_wrong_p1p2(cmd):
    sw, _ = cmd.transport.exchange(cla=0xe0,
                                   ins=0x03,
                                   p1=0x01,  # 0x01 instead of 0x00
                                   p2=0x00,
                                   cdata=b"")

    raise DeviceException(error_code=sw)


@pytest.mark.xfail(raises=WrongDataLengthError)
def test_wrong_data_length(cmd):
    # APDUs must be at least 5 bytes: CLA, INS, P1, P2, Lc.
    sw, _ = cmd.transport.exchange_raw("E000")

    raise DeviceException(error_code=sw)


# @pytest.mark.xfail(raises=WrongDataLengthError)
# def test_load_metadatas_with_too_much_data(cmd, test_vector):
#     metadatas = test_vector
#     cmd.load_metadatas(metadatas)


# @pytest.mark.xfail(raises=MetadatasParsingError)
# def test_load_metadatas_with_name_too_long(cmd, test_vector):
#     metadatas = test_vector
#     cmd.load_metadatas(metadatas)
