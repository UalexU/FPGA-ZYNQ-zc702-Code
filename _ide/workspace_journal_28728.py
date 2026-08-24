# 2026-08-20T13:26:11.625540600
import vitis

client = vitis.create_client()
client.set_workspace(path="Vitis")

platform = client.get_component(name="SPI_Sensor")
status = platform.build()

comp = client.get_component(name="SPI_Sensor_Test_8bit_V1")
comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

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

status = platform.build()

comp.build()

status = platform.build()

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

