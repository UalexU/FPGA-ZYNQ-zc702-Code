# 2026-08-21T13:58:51.443652800
import vitis

client = vitis.create_client()
client.set_workspace(path="Vitis")

platform = client.get_component(name="SPI_Sensor")
status = platform.build()

comp = client.get_component(name="SPI_Sensor_Test_8bit")
comp.build()

vitis.dispose()

