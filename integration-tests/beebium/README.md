# BBC Beebium Test Scaffold

This directory is the initial scaffold for BBC-focused integration testing of
`fujinet-nio-lib`.

The intended shape mirrors the testing split already used by `fn-rom`:

- `scripted/` for deterministic PTY/fake-device tests
- `real/` for Beebium + real `fujinet-nio` end-to-end tests

## Current scope

This first cut is intentionally small:

- the main repo `test-bbc` target builds the BBC library and BBC-supported examples
- this directory provides the dependency and fixture skeleton for future pytest lanes
- one real scripted smoke test is now checked in for a BBC library-linked C app run from SSD via `fn-rom`

## Reuse source

The design is based on these `fn-rom` assets:

- `fn-rom/integration-tests/beebium/conftest.py`
- `fn-rom/integration-tests/beebium/helpers.py`
- `fn-rom/integration-tests/beebium/fujinet_runner.py`
- `fn-rom/integration-tests/beebium/scripted/test_network_device.py`
- `fn-rom/integration-tests/beebium/real/test_real_fujinet_e2e.py`

## Planned minimal port

### Scripted lane

- Launch Beebium with `fn-rom` installed
- Let Beebium create the PTY
- Attach a fake Fuji device to the PTY
- Run a BBC C smoke app or existing example linked against `fujinet-nio-lib`
- Assert emitted network `OPEN`, `READ`, `WRITE`, `CLOSE`, and translate packets

Current scripted coverage:

- HTTP GET smoke using `apps/http_get_smoke.c`

Recommended next scripted tests:

- TCP request/response smoke
- JSON query smoke once a compact BBC test app is added

### Real lane

- Start an isolated real `fujinet-nio` instance via the same PTY topology used by `fn-rom`
- Launch Beebium attached to that PTY
- Run the BBC example or a small smoke app
- Assert on `fujinet-nio` stdout logs and, where useful, BBC screen output

Recommended first real tests:

- HTTP GET against a local `httpbin` service
- TCP echo against a local echo service

## Suggested follow-up layout

```text
integration-tests/beebium/
  pyproject.toml
  README.md
  conftest.py
  helpers.py
  fujinet_runner.py
  scripted/
    test_http_get.py
    test_tcp_get.py
  real/
    test_real_http_get.py
    test_real_tcp_get.py
  apps/
    http_get_smoke.c
    tcp_get_smoke.c
```

The `apps/` route is recommended over driving the verbose examples directly once
the test lane is implemented, because it keeps screen assertions much smaller.

## Running

The top-level make targets are:

```bash
make test-bbc
make test-bbc-scripted
make test-bbc-real
```

At present `test-bbc-scripted` and `test-bbc-real` are scaffolding targets that
expect the pytest lane to be populated.
