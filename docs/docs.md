## Runtime and baseline model

`baseline` is the last approved trusted snapshot persisted on disk.

`runtime` is the latest observed snapshot rebuilt from startup scan and filesystem events.

Rules:

- File changes update only the runtime snapshot.
- `--approve-runtime` promotes runtime to the trusted baseline and writes it to disk.
- `--reload-runtime` rebuilds runtime from the live filesystem without changing baseline.
- Metadata-only events are tracked separately from content changes when hash stays the same.
- Directory moves rekey watched paths and runtime entries so recursive coverage stays consistent.
