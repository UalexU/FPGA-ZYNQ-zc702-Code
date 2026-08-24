# 2026-08-18T10:31:30.261222300
import vitis

client = vitis.create_client()
client.set_workspace(path="Vitis")

platform = client.get_component(name="EMIO_SPI_V1")
status = platform.build()

comp = client.get_component(name="SPI_Test_V0")
comp.build()

status = platform.build()

comp.build()

status = platform.update_hw(hw_design = "$COMPONENT_LOCATION/../../design_1_wrapper_SPI_V1.xsa")

status = platform.build()

status = platform.build()

comp.build()

status = platform.update_hw(hw_design = "$COMPONENT_LOCATION/../../design_1_wrapper_SPI_V1.xsa")

status = platform.build()

advanced_options = client.create_advanced_options_dict(dt_overlay="0")

platform = client.create_platform_component(name = "EMIO_SPI_8Bit",hw_design = "$COMPONENT_LOCATION/../../design_1_wrapper_SPI_V1.xsa",os = "standalone",cpu = "ps7_cortexa9_0",domain_name = "standalone_ps7_cortexa9_0",generate_dtb = False,advanced_options = advanced_options,compiler = "gcc")

comp = client.create_app_component(name="SPI_Test",platform = "$COMPONENT_LOCATION/../EMIO_SPI_8Bit/export/EMIO_SPI_8Bit/EMIO_SPI_8Bit.xpfm",domain = "standalone_ps7_cortexa9_0")

platform = client.get_component(name="EMIO_SPI_8Bit")
status = platform.build()

client.delete_component(name="SPI_Test")

comp = client.create_app_component(name="Spi_Test",platform = "$COMPONENT_LOCATION/../EMIO_SPI_8Bit/export/EMIO_SPI_8Bit/EMIO_SPI_8Bit.xpfm",domain = "standalone_ps7_cortexa9_0")

status = platform.build()

comp = client.get_component(name="Spi_Test")
comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

vitis.dispose()

