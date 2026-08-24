# 2026-08-13T13:53:30.325964400
import vitis

client = vitis.create_client()
client.set_workspace(path="Vitis")

platform = client.get_component(name="platform")
status = platform.build()

comp = client.get_component(name="hello_world")
comp.build()

vitis.dispose()

