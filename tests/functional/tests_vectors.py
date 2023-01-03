EXISTING_METADATA = b"\n\x00\x07password1\n\x00\x07password2\n\x00\x07password3"

tests_vectors = {
    "test_generate_password": [
        [0x01, "gmail", "HMYDQUIOVKPCKJIHQJEN"],
        [0x03, "gmail", "KqIJcPjhENivHvOdmuKQ"],
        [0x07, "gmail", "xNX8IQO4vP0ucO41J6JW"],
        [0x0F, "gmail", "w14JrbA9HNvWU1ON5MGP"],
        [0x1F, "gmail", "vy4Joa86FKvVS1ON4KEP"],
        [0x3F, "gmail", "kD83CP1UZO vQvJIuNx4"],
        [0x7F, "gmail", "?u8htP1|DO v7vJzYNb4"],
        [0xFF, "gmail", "*m8ZlP1|}O vzvJrQNT4"],
        [0xFF, "aseedoflengthequal20", "29!uO;UPx UT8Hkmi- 5"],
        [0xFF, "aSeedOfLengthEqual20", " $4,P.usI*C\\k1fv2;M;"]],

    "test_dump_metadatas": [
        [0, b""],
        [100, EXISTING_METADATA + b"\x00" * (100 - len(EXISTING_METADATA))],
        [4096, EXISTING_METADATA + b"\x00" * (4096 - len(EXISTING_METADATA))]
    ],

    "test_load_metadatas": [
        # 1-element array to avoid huge test names filled with the data.
        # Instead, it is filled with the data index
        [b"\x00" * 4096],
        [bytes.fromhex("02000761060007616c6c6168")],
        [bytes.fromhex("02000761060007616c6c6168") + b"\x00" * (4096 - 12)],
        [bytes.fromhex("02000761" "14 00 07 616c6c6168616c6c6168616c6c6168616c6c70") +
        b"\x00" * (4096 - 26)],
    ],

    "test_load_metadatas_with_too_much_data": [
        b"\x00" * 10000,
        bytes.fromhex("02000761060007616c6c6168") + b"\x00" * 4096,
    ],

    "test_load_metadatas_with_name_too_long": [
        bytes.fromhex(
            "02000761" "15 00 07 616c6c6168616c6c6168616c6c6168616c6c7078"),
        bytes.fromhex("02000761" "15 00 07 616c6c6168616c6c6168616c6c6168616c6c7078") +
        b"\x00" * (4096 - 27),
    ],
}
