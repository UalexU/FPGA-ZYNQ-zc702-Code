# 2026-08-25T10:35:30.187588100
import vitis

client = vitis.create_client()
client.set_workspace(path="Vitis")

platform = client.get_component(name="2_Sensors_dw")
status = platform.build()

vitis.dispose()

