# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.3.1] - 2026-05-25

### Fix

- Reject password creation when the chosen nickname is already in use.
- Hide actions that require existing passwords from the main menu when the
  database is empty (only "Create a new password" is exposed in that case).
- Settings switches on the second page (separators, special characters, "press
  enter") no longer display a stale value after toggling and re-entering the
  settings.

### Change

- Reorder the main menu so "Create a new password" appears first.

### Add

- Functional tests support Nano S+ / Nano X (Ragger button-based navigator).
- Unit tests for the new `nickname_exists` check.

## [1.3.0] - 2025-09-11

### Change

- Realign App to the latest SDK and nbgl API
- Remove Nanos support
- Port to full Standard App framework
- Add Flex support
- Add Apex_p support
- Use NBGL on Nano

## [1.2.1] - 2025-07-24

### Change

- Adapt WUI to new DNS
- Update ragger tests after deprecation of `Firmware`
- Compatibility with io_revamp

### Add

- Startup disclaimer on Stax
- CNAME file so that GitHub correctly sets the `passwords.ledger.com` DNS of the WUI tool

## [1.2.0] - 2024-03-13

### Fix

- Bug #38: some applications were randomly lowering capital characters, leading passwords containing
  capital characters to be almost always wrong.

### Change

- Move to standard app framework

### Add

- Integrate WUI backup/restore tool inside the app repository

## [1.1.2] - 2023-10-12 (Stax only)

### Fix

- Updated porting to Stax SDK evolutions

## [1.1.1] - 2023-04-25 (Stax only)

### Fix

- Derivation path changed, from `44'/1` to `5265220'`

## [1.1.0] - 2023-04-12 (Stax only)

### Add

- Stax porting

## [1.0.2] - 2022-03-02

Original Passwords application
