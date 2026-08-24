# 2026-08-21T14:33:00.475221900
import vitis

client = vitis.create_client()
client.set_workspace(path="Vitis")

platform = client.get_component(name="SPI_Sensor")
status = platform.build()

comp = client.get_component(name="SPI_Sensor_Test_8bit")
comp.build()

