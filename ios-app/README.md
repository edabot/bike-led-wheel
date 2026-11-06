# led-wheel-controller

SwiftUI iPhone app that picks the effect shown on the bike LED wheels. It connects over BLE to one wheel, and that wheel passes the command on to the other wheel. See the [repo README](../README.md) for the firmware side.

## Building

Open `led-wheel-ble.xcodeproj` in Xcode and run the **iOS** target on a real iPhone. The Simulator has no Bluetooth.

- Deployment target: iOS 14.0
- Bundle id `supamoto.ble-test`. To sign with your own account, change the team and bundle id.
- Bluetooth permission text is set in `iOS/Info.plist` (`NSBluetoothAlwaysUsageDescription`).

## Using it

1. Power the wheels, then tap **Scan**. The app scans for 5 seconds and lists only devices whose names start with `Itsy` (v2 wheels advertise as `Itsy-Wheel-A` / `Itsy-Wheel-B`).
2. Tap a wheel to connect. The app opens the wheel editor.
3. Pick a mode, a palette/colour and optionally **Reverse**, then tap **Send It**.

## Code layout

| File | What |
|---|---|
| `Shared/ble_testApp.swift` | App entry point. |
| `Shared/ContentView.swift` | Scan / device list / connection status screen. |
| `Shared/BLEManager.swift` | CoreBluetooth central: scan, connect, find the UART characteristics, write commands. |
| `Shared/CBUUIDs.swift` | Nordic UART service and characteristic UUIDs. |
| `Shared/Model/BlePeripheral.swift` | Static holder for the connected peripheral and characteristics. |
| `Shared/WheelEditor.swift` | Per-wheel mode, submode and reverse pickers, plus the Send button. |
| `Shared/WheelData.swift` | Wheel state and building the command string. |
| `Shared/Model/ModeConstants.swift` | Mode and submode names shown in the pickers. |

## Protocol

The app writes ASCII to the Nordic UART TX characteristic (`6E400002-…`, with response):

```
M <mode> <submode char> X
```

- `mode`: `0` pattern, `1` fireball, `2` sparks, `3` double fireball, `4` front fire
- `submode char`: `'0' + index`, plus 30 if reversed

The firmware also accepts two commands in one write (`M12XM05X`): the first goes to wheel A and the second to wheel B. See the repo README for that and for the debug commands.

## Keeping in sync with the firmware

The submode ids in `ModeConstants.swift` are indexes into the palette tables in `firmware/bike_wheel_lights_v2/palettes.h`. Keep the order the same on both sides. The firmware has `static_assert`s on the table sizes (15 patterns, 3 fireballs, 4 sparks), so adding a palette means updating both places.

## Known limitations

- **Only wheel one's settings are sent.** `makeSendString()` builds a single `M..X` command that every wheel applies. The two-command version for dual mode is commented out, so the **Dual mode** toggle and the wheel two pickers currently do nothing.
- `WheelEditor` creates its own `BLEManager`. Sending still works because writes go through the static `BlePeripheral`, but connection status isn't shared between the two screens.
