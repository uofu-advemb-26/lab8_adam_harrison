# Lab 8: CAN Bus

* **High priority node** (`combined_high_priority`): transmits ID `0x010`
* **Low priority node** (`combined_low_priority`): transmits ID `0x200`

| Busy wait (µs) | High priority frames received/s | Low priority frames sent | Result |
|---:|---:|---:|---|
| 0 | ~4,756 | 0 | Starved, transmit queue full (can2040_transmit returns -1) |
| 100 | ~4,756 | 0 / 15 s | Starved |
| 190 | ~4,756 | 0 / 14 s | Starved |
| 200 | ~4,631 | 14 / 14 s | Every message gets through |

### Observations

* Between 180 and 190 µs the results weren't consistent (4 frames once, then 0)
* Attaching scope probes to both CANH and CANL at the same time stopped all traffic on the bus. With a probe on CANH alone the bus kept working. The second probe was most likely shorting CANL (pin 6) to a neighboring pin or to ground.
