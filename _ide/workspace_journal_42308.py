# 2026-08-20T15:22:02.782356400
import vitis

client = vitis.create_client()
client.set_workspace(path="Vitis")

client.delete_component(name="EMIO_SPI")

client.delete_component(name="EMIO_SPI")

client.delete_component(name="SPI_Sean")

client.delete_component(name="SPI_Sean")

client.delete_component(name="EMIO_SPI_V1")

client.delete_component(name="EMIO_SPI_V1")

vitis.dispose()

