# NTP sync: why e2e_delay_us goes negative, and how it's mitigated

## Symptom

`e2e_delay_us` is negative, or swings wildly (hundreds to thousands of µs) between otherwise
identical back-to-back runs — most visible on loopback (`--host 127.0.0.1`) or same-LAN tests,
where the real transport delay is only tens to hundreds of microseconds.

## Root cause

`e2e_delay_us` is NTP-corrected to account for sender/receiver clocks not reading the same wall
time:

```
e2e = (ts_received - ts_sent) + (receiver_offset - sender_offset)
```

Each offset comes from one SNTP (RFC 4330) exchange with a time server (`src/sync/NtpSync.cpp`,
pre-mitigation). SNTP's offset formula —

```
offset = ((T2-T1) + (T3-T4)) / 2
```

— assumes the request and response legs of the path take equal time. Any real-world asymmetry
(different routing per direction, a queued router hop, WiFi contention) turns directly into offset
error of the same size as the asymmetry. Against a public server (`pool.ntp.org`) over a typical
network path, that's commonly hundreds of microseconds to several milliseconds.

Sender and receiver each run this exchange independently, so even when they're the exact same
physical machine (loopback test — true offset is 0 by construction), each gets its own random
error, and `receiver_offset - sender_offset` inherits both. On loopback/LAN, where the real
transit delay is only tens-to-hundreds of µs, this correction term can be several times larger
than the signal it's supposed to refine — and its sign is arbitrary, so `e2e_delay_us` comes out
negative about as often as inflated.

### Empirical confirmation

Five consecutive loopback TCP runs, same host, same config, NTP enabled by default:

| Run | offset diff (sender−receiver) | `e2e_mean_us` (raw) | `e2e_mean_us` − diff (true transit) |
|---|---|---|---|
| 1 | +444 µs | −285.0 | 159.4 |
| 2 | −304 µs | 506.9 | 202.9 |
| 3 | −3594 µs | 3782.8 | 189.2 |
| 4 | −2684 µs | 2855.7 | 172.1 |
| 5 | −4740 µs | 4846.8 | 106.7 |

The raw mean swings from −285 µs to +4847 µs run to run — same code, same host — purely because
each launch re-queries NTP independently. Subtracting each run's own offset diff collapses all
five to a stable ~107–203 µs, matching a `--no-ntp` control run (281.8 µs, no correction, pure
`ts_received - ts_sent`). This is the definitive signature of NTP query noise, not a timestamping
or formula bug.

## Reproducing it

```bash
# Control: --no-ntp on both sides isolates raw timestamping from NTP noise.
./build/pb-tool --mode receiver --protocol tcp --port 7101 --no-ntp --run-id ctrl &
./build/pb-tool --mode sender --protocol tcp --host 127.0.0.1 --port 7101 --no-ntp \
    --payload-format random --payload-size 512 --rate 1000 --duration 10 --warmup 2 \
    --run-id ctrl
# Expect: e2e_mean_us small and positive, every run.

# Repeat 5x WITHOUT --no-ntp and WITHOUT --ntp-skip-loopback (default flags), same host/config:
# Expect: e2e_mean_us swings between runs, sometimes negative — each launch's independent
# NTP query offset diff (packets.csv columns ntp_offset_sender_ns / ntp_offset_receiver_ns,
# constant per run) explains essentially all of the swing.

# Add --ntp-skip-loopback to the sender command above and repeat 5x:
# Expect: e2e_mean_us stable and positive across all runs (see confirmation below).
```

## Mitigations

The multi-sample sync and `--ntp-server` resolution are on by default. `--ntp-skip-loopback` is
opt-in (off by default) since it changes what gets measured — see its section below for why.

### 1. Multi-sample, min-RTT-filtered SNTP (`src/sync/NtpSync.cpp`)

`NtpSync::query(server, samples=8, timeout)` performs `samples` independent exchanges instead of
one, and keeps the sample with the lowest measured RTT (`(T4-T1)-(T3-T2)`) — the standard NTP
client heuristic, since a smaller RTT statistically implies a more symmetric (less erroneous)
path. Returns `NtpInfo{offset_ns, uncertainty_ns}`, where `uncertainty_ns` is half that sample's
RTT: an explicit upper bound on the offset error, not just the offset itself.

### 2. Loopback skip — `--ntp-skip-loopback` (opt-in, off by default)

`app/main.cpp::is_loopback_host()` resolves `--host` (via `getaddrinfo`, so `/etc/hosts` aliases
are covered, not just literal `127.0.0.1`/`localhost`); with `--ntp-skip-loopback` passed to the
sender, a loopback target skips NTP entirely — same physical clock as the receiver, true offset is
0, any correction can only add noise. `WireHeader.flags` bit 3 (`is_ntp_disabled()`) carries this
decision to the receiver over the wire, so `compute_e2e_delay_us` (`include/pbt/metrics/
PacketMetrics.hpp`) ignores **both** sides' offsets for that run — including a receiver that
independently ran a real, nonzero-offset NTP query of its own. Only the sender needs the flag;
`--no-ntp` is not required on either side.

Off by default because it changes the measurement, not just the noise floor: passing it means
asserting "sender and receiver are the same clock," which is only true for genuine loopback tests.
Leave it off for anything you want the normal (already-mitigated) NTP path to measure.

### 3. `--ntp-server` / local-first resolution (`app/main.cpp`)

`--ntp-server <host>` (or `PBT_NTP_SERVER` env) pins an explicit server. Without one, resolution
tries `127.0.0.1` first (2 samples, 300 ms timeout — cheap even when nothing is listening there),
falling back to `pool.ntp.org` only if that fails. A local time daemon has a far shorter, more
symmetric network path than a public server, so its offset estimate is proportionally far more
accurate — worth trying before paying the public-server error budget.

### 4. Sync-uncertainty reporting

Both sides' `uncertainty_ns` travel alongside their offsets (sender's over the wire in
`WireHeader.ntp_uncertainty_ns`; receiver's locally in `InboundPacket`). `PacketProcessor` sums
them into `ntp_sync_uncertainty_us` — the ± band on `e2e_delay_us` attributable to clock-sync
error — exposed per-row in `packets.csv` and once per run in `summary.json["ntp"]`. A run's
`e2e_delay_us` should not be trusted at a finer resolution than its `sync_uncertainty_us`.

## What this does NOT fix

None of the above turns SNTP into a microsecond-accurate protocol — it reduces and *quantifies*
the error, it doesn't eliminate it. For genuinely sub-millisecond cross-machine correction (rare
outside dedicated time-sensitive-networking setups), the only real answer is PTP (IEEE 1588) with
hardware timestamping, which is out of scope for this tool. For WAN benchmarks where the expected
transport delay is already milliseconds or more, the residual SNTP error is proportionally small
and the mitigations above are sufficient.
