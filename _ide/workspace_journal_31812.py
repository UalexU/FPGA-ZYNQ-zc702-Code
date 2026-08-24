# 2026-08-18T14:00:05.441397600
import vitis

client = vitis.create_client()
client.set_workspace(path="Vitis")

platform = client.get_component(name="EMIO_SPI_8Bit")
status = platform.update_hw(hw_design = "$COMPONENT_LOCATION/../../design_1_wrapper.xsa")

status = platform.build()

status = platform.build()

comp = client.get_component(name="Spi_Test")
comp.build()

component = client.get_component(name="Spi_Test")

lscript = component.get_ld_script(path="C:\Users\Admin\XilinxProjects\Test\Vitis\Spi_Test\src\lscript.ld")

lscript.regenerate()

client.delete_component(name="Spi_Test")

vitis.dispose()

