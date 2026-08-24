# 2026-08-18T13:05:00.468441500
import vitis

client = vitis.create_client()
client.set_workspace(path="Vitis")

platform = client.get_component(name="EMIO_SPI_8Bit")
status = platform.build()

comp = client.get_component(name="Spi_Test")
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

