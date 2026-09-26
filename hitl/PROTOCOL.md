# SimpleBLE fixture protocol, version 1

The fixture targets PCA10056 / nRF52840 with S140 7.2.0. It supports one
peripheral connection, disables bonding, and rejects pairing requests. All
multibyte integers are little-endian. The GATT table remains fixed across
advertising-profile changes and resets.

## Identity and advertising

The full board ID is `(FICR.DEVICEID[1] << 32) | FICR.DEVICEID[0]`, printed as
16 lowercase hex digits. Name: `SBH-XXXXXXXX`, uppercase low 32 bits. The stable
random-static address uses the SoftDevice's factory-derived default, XORs the
low address byte with `0x48`, and sets the high two address bits.

Legacy advertising uses 1M PHY, 100 ms interval, and 0 dBm. It starts on boot
and disconnect and stops on connection. Profile changes take effect when
advertising next starts.
The token is an unsigned 32-bit integer, independent of the RESET test ID.
Company ID `0xffff` is used for this local test fixture.

Let `U = 01 43 5f 6e 8d 9c b0 a1 6f 4b 7a 2e 01 00 57 7e` (service UUID,
wire order) and `T` be the four token bytes. Exact AD structures, including
length/type bytes:

| Profile | Advertising packet | Scan response |
| --- | --- | --- |
| `default` | `02 01 06 11 07 U 08 ff ff ff 01 T` (30 bytes) | `0d 09 <12 ASCII name bytes> 02 0a 00` (17 bytes) |
| `service_data` | `02 01 06 16 21 U 01 T` (26 bytes) | Same as default |
| `no_tx_power` | Same as default | `0d 09 <12 ASCII name bytes>` (14 bytes) |
| `nonconnectable` | Same as default | Same as default; scannable, nonconnectable |
| `nonscannable` | Same as default | None; nonconnectable and nonscannable |

The C++ client selects candidates by name and service UUID. INFO and HELLO
expose the full board ID; RTT attachment also reads the ID directly from FICR.

## GATT

Custom UUIDs are `7e57xxxx-2e7a-4b6f-a1b0-9c8d6e5f4301`. The primary service is
suffix `0001`.

| Suffix | Control name | Properties | Initial / maximum bytes |
| --- | --- | --- | --- |
| `0002` | INFO | Read | 16 / 20 |
| `0003` | READ_VALUE | Read | `00 01 ... 1f` / 512 |
| `0004` | WRITE_REQUEST | Read, Write Request | empty / 512 |
| `0005` | WRITE_COMMAND | Write Without Response | empty / 512 |
| `0006` | NOTIFY_A | Read, Notify | empty / 244 |
| `0007` | NOTIFY_B | Read, Notify | empty / 244 |
| `0008` | INDICATE | Read, Indicate | empty / 244 |
| `0009` | ERROR | Read, Write Request | both reject with ATT `0x80` |
| `0010` | DESC | Read/write descriptor on READ_VALUE | `44 45 53 43` / 32 |

INFO contains `version:u32, board:u64, test_id:u32`. READ_VALUE also has the
read-only User Description `0x2901`, exactly `SimpleBLE HITL read` (19 bytes,
no terminator). Notify/indicate characteristics have standard CCCDs.
Battery Service `0x180f` / Battery Level `0x2a19` is Read/Notify, control name BATTERY,
initially 50; its values must be exactly one byte in 0–100.

Binary lengths are preserved. Successful writes replace values, including a
shorter replacement. Long reads are supported through 512 bytes. Ordinary
writes and sends are limited to `min(ATT_MTU-3, attribute maximum)`.
Prepared/execute writes are rejected with ATT application error `0x82`;
canceling prepared writes is acknowledged without changing any value.
Invalid offsets/lengths are rejected. Write Commands have no ATT response;
RTT records them when the stack exposes them. Stack-discarded invalid commands
may not produce an application event.

## Control transports

The command grammar below is shared by RTT and BLE. Use one controller and one
outstanding command at a time.

RTT uses channel 0 for commands, replies, and asynchronous events.

BLE uses a separate primary service, suffix `0011`, on the same peripheral
connection:

| Suffix | Attribute | Properties | Maximum bytes |
| --- | --- | --- | --- |
| `0012` | Control input | Write Request | 244, limited by ATT MTU |
| `0013` | Control response page | Read | 20 |

Control-input writes have one of two formats:

- `00 <ASCII bytes>` appends a command fragment. LF terminates and executes the
  command. The assembled line is limited to 1536 bytes including LF. Invalid
  input is discarded through the next LF and returns `0 ERR code=line`.
- `01 <offset:u16le>` selects a byte offset in the retained reply.

Each response read returns `total_length:u16le, offset:u16le, data[0..16]`.
The host selects offset zero, reads a page, and advances by the number of data
bytes until it has the complete reply. Replies include LF and remain available
until the next command. Response reads use ATT offset zero; paging is selected
through the control-input characteristic. Partial commands are cleared on
BLE disconnect.

For example, write `00 31 20 48 45 4c 4c 4f 0a` to issue `1 HELLO`, then write
`01 00 00` and read the response page. The C++ client fragments commands into
19-byte ASCII segments and uses ordinary reads, so control works at ATT MTU 23.
BLE control accepts LF line endings; RTT also accepts CRLF.

BLE replies carry command results. `GET_WRITE`, `WORK`, and `STATUS` expose
write history, delivery counts, and connection state. Asynchronous event records
remain on RTT.

## Control commands

Commands use newline-terminated ASCII, at most 1536 bytes including LF.
Decimal integers are unsigned 32-bit with no signs.
Hex arguments are case-insensitive, even-length, and use `-` for empty bytes.
Attribute names and command/profile names are case-sensitive as shown here.
Only one request is outstanding. Request IDs are nonzero decimal integers.
Replies are `<id> OK [key=value ...]` or `<id> ERR code=<reason>`; asynchronous
`EV` records may interleave. Malformed lines without a usable ID reply with ID 0.

| Command | Meaning / reply fields |
| --- | --- |
| HELLO | `version board boot build max_value max_send line` |
| STATUS | `boot conn test connected advertising mtu ceiling active accepted tx confirmed dropped cccd_a cccd_b cccd_i cccd_battery` |
| RESET test_id | Cancel work/schedule, disconnect, restore initial values/profile/token/MTU ceiling, restart advertising; reply `test` |
| ADV profile token [duration_ms] | Select an advertising profile; apply immediately while disconnected or on the next disconnect |
| MTU 23\|247 | Set the next connection's MTU ceiling |
| SET attribute hex | Replace a value without transmitting; INFO/ERROR are immutable |
| GET attribute | Current `len writes data` |
| GET_WRITE attribute | Last accepted BLE write's `len writes op data`, retained across SET; `writes=0, op=0` means none |
| WORK attribute | Current or most recent send/stream: `active stream sent completed reason`; cleared by RESET |
| SEND attribute hex | Queue one notification/indication; completion is a separate event |
| STREAM attribute stream_id length count interval_ms | Custom NOTIFY_A/B or INDICATE; length 12–244, count 1–10000, interval 20–1000 ms |
| STOP | Cancel application sends/streams; reply after application queues are cleared |
| DISCONNECT delay_ms | Schedule GAP disconnect; requires a connection |
| REBOOT delay_ms | Schedule MCU reset without a preceding GAP disconnect |

DISCONNECT and REBOOT delays are 100–10000 ms. One scheduled action is allowed;
RESET cancels it. For BLE commands, the delay begins when the host reads the
final response page. BLE RESET is acknowledged before disconnecting and executes
100 ms after that read. The host then reconnects to observe the reset state.
RTT RESET replies after completion; RTT scheduled-action delays start when the
command is accepted.

An optional ADV duration of 1000–60000 ms restores the default connectable
profile after that advertising interval, retaining the token. BLE control
requires a duration for `nonconnectable` and `nonscannable`, allowing the host
to reconnect after the test. MTU changes preserve the current connection's
negotiated limit.

Reboot changes the random 64-bit boot nonce; logical RESET preserves boot and
connection numbering.

The build ID is SHA-256 over the SDK archive digest and firmware/build files.
The flash report records the SHA-256 of each programmed image.

Errors: `syntax`, `line`, `attribute`, `immutable`, `length`, `subscription`,
`busy`, `disconnected`, `resetting`. Rejected commands do not partially
modify values. SET/SEND/STREAM conflict with existing work on the same attribute.

Stream packets: `test_id:u32, stream_id:u32, sequence:u32`, followed at each offset
`i >= 12` by `(sequence+i)&255`. Sequence starts at zero. One stream per custom
sending characteristic. Backpressure retries do not advance the sequence.
The connection permits one outstanding notification and one outstanding
indication. The interval specifies minimum spacing between stack acceptances.
Successful sends update the readable value; SET never transmits.

STOP, unsubscribe, RESET and disconnect cancel application work. Packets already
accepted by the stack may complete afterward; drain host callbacks before a new
phase. A canceled attribute cannot start new work until its accepted packet
finishes. Reconnection never resumes old streams.

## Events and counters

Each event starts with `EV boot=<16hex> conn=<u32> test=<u32> seq=<u32>`.
Connection ID increases per boot, starting at 1. Event sequence starts at 1 and
continues through logical RESET. Event fields by `type`:

- `boot`: `board build`
- `connected`: no additional fields
- `disconnected`: GAP `reason` (decimal)
- `mtu`: negotiated raw ATT `mtu`
- `write`: `attr op len offset accepted data`; op 1=request, 2=command,
  4=prepare; data is the exact observed hex payload
- `cccd`: `attr value` (0=off, 1=notify, 2=indicate)
- `complete`: `attr kind stream sent completed reason`; kind `send` or `stream`,
  stream ID 0 for SEND; reason `done`, `stop`, `unsubscribe`, `reset`,
  `disconnect`, or `error`
- `send_error`: `attr code` (SoftDevice error)
- `scheduled`: `action` (`disconnect` or `reboot`)
- `fault`: `id pc info code line`; firmware stops for diagnosis

`sent` means stack-accepted. `completed` means notification TX completion or
indication confirmation, depending on the attribute. Cancellation summaries
are snapshots; already accepted packets can complete later. STATUS separately
counts all accepted packets, notification TX completions, and indication
confirmations. RESET clears those counters and write history. `dropped` counts
lost whole RTT records for the entire boot and is not cleared by RESET. A
regression run using RTT checks for increases in this count. BLE-only runs use
queried state and host callbacks.

RTT writes use nonblocking skip mode: a complete record fits or the entire
record is dropped. Firmware continues without a reader. Stream packets are not
individually logged. Scheduled MCU reset can erase its final log records;
the C++ RTT client detaches before reset and reattaches afterward so J-Link
discards the previous boot's buffer offsets.
