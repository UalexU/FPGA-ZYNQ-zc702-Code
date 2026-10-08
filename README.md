# TE0745-03-92I31-A (Zynq-7000) — TMAG5170UEVM

| Folder | What it is | Open with |
|---|---|---|
| `hw/` | Vivado project (`SPI_TMAG5170UEVM_design_1` block design, constraints) | Vivado → open `hw/TE0745-03-92I31-A.xpr` |
| `xsa/` | Exported hardware goes here (create it on the next export) | — |
| `fw/` | Vitis workspace: `hello_world`, `LED_BTN_TEST_NoDrivers`, `SPI_Sensor_Test_8bit` | Vitis → Open Workspace → `fw/` |
| `host/` | PC-side Python: `GUI.py`, `sensor.py`, `run_board.py` | Python |

Note: the apps reference platforms `GPIO_Platform` / `SPI_Sensor` that are not in this workspace —
recreate a platform from the exported .xsa and point the apps at it.

## Python setup (once)
```
cd host
python -m venv .venv
.venv\Scripts\activate
python -m pip install -r requirements.txt
python GUI.py
```
