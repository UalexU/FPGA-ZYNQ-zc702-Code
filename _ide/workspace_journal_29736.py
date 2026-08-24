# 2026-08-21T09:43:58.580542
import vitis

client = vitis.create_client()
client.set_workspace(path="Vitis")

platform = client.get_component(name="GPIO_Platform")
status = platform.build()

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

status = platform.build()

comp.build()

status = platform.build()

comp.build()

vitis.dispose()

