# 2026-08-19T10:10:48.448471700
import vitis

client = vitis.create_client()
client.set_workspace(path="Vitis")

platform = client.get_component(name="SPI_Sean")
status = platform.build()

comp = client.get_component(name="SPI_Seanw")
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

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

vitis.dispose()

