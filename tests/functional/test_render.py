from pathlib import Path

from passwordsManager_cmd import PasswordsManagerCommand


def test_backup_render(cmd: PasswordsManagerCommand, default_screenshot_path: Path):
    # Triggering a backup makes the device show "Backup password list?".
    # Capture that approval screen, then approve.
    cmd.dump_metadatas(1, compare=(default_screenshot_path, "backup"))
    cmd.reset_approval_state()


def test_restore_render(cmd: PasswordsManagerCommand, default_screenshot_path: Path):
    # Triggering a restore makes the device show "Restore password list?".
    # Capture that approval screen, then approve. The payload is a small valid
    # metadata blob (one entry), shared with the load test vectors.
    metadatas = bytes.fromhex("02000761060007616c6c6168")
    cmd.load_metadatas(metadatas, compare=(default_screenshot_path, "restore"))
    cmd.reset_approval_state()
