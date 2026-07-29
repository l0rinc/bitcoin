# Discovery Review Shard 01

- Assigned inventory range: lines 1-822 inclusive
- Assigned paths: 822
- Receipt rows: 822
- Reviewed text/source/configuration files: 733
- Binary/generated/declaration/source-map files accounted: 89
- Unreadable files: 0
- Raw candidates: 5

## Review coverage

Every assigned file was opened from start to finish. The pass covered CI expression and command construction, host/container effects, cache and artifact trust, build/dependency integrity, archive extraction, release signing and attestation, filesystem/path safety, developer-tool parsing and RPC/network behavior, denial-of-service bounds, documentation active content and credential examples, generated artifacts, and untracked third-party JavaScript parser/auth/server code. The only binary was `doc/bitcoin_logo_doxygen.png`; file type and metadata were assessed. All 821 textual files decoded as UTF-8; all 35 Python entrypoints/modules parsed, all 31 JSON/source-map artifacts parsed, and assigned shell sources passed syntax checks.

## Classification counts

- `audit_journal`: 56
- `audit_probe_source`: 8
- `automation_prompt_or_catalog`: 3
- `binary_image_asset`: 1
- `build_configuration`: 41
- `ci_automation`: 11
- `ci_build_tooling`: 45
- `contributed_tool_or_support`: 71
- `dependency_build_recipe`: 47
- `dependency_patch`: 32
- `documentation`: 217
- `generated_build_helper`: 2
- `generated_dependency_artifact`: 86
- `local_catalog_generator`: 1
- `release_build_tooling`: 29
- `release_security_tool`: 19
- `repository_configuration`: 6
- `repository_documentation`: 5
- `repository_template`: 5
- `service_configuration`: 7
- `untracked_third_party_metadata`: 55
- `untracked_third_party_runtime`: 75

## Raw candidates

1. `contrib/verify-binaries/verify.py`: predictable shared work directory permits pre-positioned filesystem entries during release verification.
2. `contrib/macdeploy/detached-sig-create.sh`: macOS signing/notarization passphrases are passed in child argument vectors.
3. `contrib/windeploy/detached-sig-create.sh`: Windows signing passphrase is passed in the `osslsigncode` argument vector.
4. `contrib/signet/getcoins.py`: remote CAPTCHA retrieval/conversion lacks timeout and response-size bounds.
5. `ci/test/02_run_container.py`: predictable shared environment-file creation is not exclusive or symlink-safe.

## Unreadable files

None.
