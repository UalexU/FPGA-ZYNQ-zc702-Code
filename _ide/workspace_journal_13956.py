# 2026-08-19T15:18:47.392066700
import vitis

client = vitis.create_client()
client.set_workspace(path="Vitis")

comp = client.create_app_component(name="SPI_Sensor_Test_8bit",platform = "$COMPONENT_LOCATION/../SPI_Sensor/export/SPI_Sensor/SPI_Sensor.xpfm",domain = "standalone_ps7_cortexa9_0")

platform = client.get_component(name="SPI_Sensor")
status = platform.build()

comp = client.get_component(name="SPI_Sensor_Test_8bit")
comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

vitis.dispose()

