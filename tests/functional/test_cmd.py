def test_app_info(cmd):
    assert cmd.get_app_info() == ("Passwords", "1.2.0")


def test_app_config(cmd):
    assert cmd.get_app_config() == (4096, 0, 0)


def test_generate_password(cmd, test_vector):
    charset, seed, expected = test_vector
    assert cmd.generate_password(charset, seed) == expected


def test_dump_metadatas(cmd, test_vector):
    size, expected = test_vector
    assert cmd.dump_metadatas(size) == expected
    cmd.reset_approval_state()


def test_load_metadatas(cmd, test_vector):
    # [0] to avoid huge test names filled with the data.
    # Instead, it is filled with the data index
    metadatas = test_vector[0]
    cmd.load_metadatas(metadatas)
    assert cmd.dump_metadatas(len(metadatas)) == metadatas
    cmd.reset_approval_state()
