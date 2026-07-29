# Discovery review shard 06

- Inventory range: lines 4111-4932 inclusive
- Exact receipt count: 822
- First path: `src/base58.h`
- Last path: `src/qt/forms/sendcoinsdialog.ui`
- Complete bytes read/accounted: 9652739
- Raw candidates: 6
- Missing or unreadable files: 0

## Review disposition

- `binary_generated_accounted`: 7
- `metadata_accounted`: 1
- `reviewed`: 814

## Classifications

- `binary_image_asset`: 5
- `binary_runtime_data`: 1
- `binary_test_fixture`: 1
- `build_configuration`: 12
- `documentation`: 39
- `empty_accounted_file`: 1
- `generated_ui_definition`: 16
- `production_header`: 248
- `production_source`: 195
- `python_tooling`: 1
- `repository_data_or_text`: 41
- `shell_tooling`: 7
- `test_or_benchmark`: 76
- `vendored_runtime`: 169
- `vendored_test_or_example`: 10

## Raw candidate instances

- `http-incremental-chunked-body-reparse`: Incomplete chunked HTTP bodies are reparsed and recopied from the beginning on every receive event, while ignored chunk extensions and framing are not covered by the 32 MiB decoded-body limit.
- `http-per-client-pipeline-queue`: A client may continue pipelining complete HTTP requests into an uncapped per-client queue while an earlier request is busy.
- `external-signer-unsigned-transaction-substitution`: The external signer response can replace the entire unsigned transaction after the host-side send confirmation, because the returned PSBT is not compared with the requested PSBT.
- `psbt-global-unsigned-tx-value-boundary`: The PSBT global unsigned-transaction value is deserialized from the outer stream before its declared value length is enforced, permitting cross-field parsing and allocation before rejection.
- `psbt-input-non-witness-utxo-value-boundary`: A PSBT input non-witness-UTXO value is deserialized from the outer stream before its declared value length is enforced, permitting cross-field parsing and allocation before rejection.
- `qt-wallet-csv-formula-injection`: CSV exports quote user-controlled wallet labels but do not neutralize spreadsheet formula prefixes, allowing exported cells to be interpreted as formulas.

All 822 assigned files were read from start to finish. Text and source files were reviewed for their applicable threat-model boundaries; opaque binary and generated assets were read byte-for-byte and metadata-accounted. Tests, examples, generated UI files, vendored runtime code, and fixtures were not excluded.

Historical journal findings were checked before candidate recording. In particular, the already-fixed Taproot BIP32 PSBT boundary bug and oversized P2P locator issue were not duplicated. The two current PSBT rows are distinct generic-helper transaction instances.
