# Keys

**English** | [简体中文](README_CN.md)

Read all five keys independently, without Factory navigation or PASS/FAIL actions.

```bash
pio run -e keys -t upload
pio device monitor -b 115200
```

KEY1–5 use GPIO10/14/13/12/0 and are active-low. This demo enables ALDO3 but does not initialize the displays.

Press and hold each key in turn. The console should report pressed, released/clicked, and long-press events, and update the `seen` bitmask. The mask reaches `0x1F` after all five keys have been detected. KEY4 does not trigger software sleep in this demo.

KEY5 uses GPIO0, which is also a boot-strapping pin, so release it during reset. If inputs do not change, check the pin mapping, cable, and ALDO3 power rail.
