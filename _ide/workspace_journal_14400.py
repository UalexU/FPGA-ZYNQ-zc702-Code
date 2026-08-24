# 2026-08-17T09:35:22.112178800
import vitis

client = vitis.create_client()
client.set_workspace(path="Vitis")

advanced_options = client.create_advanced_options_dict(dt_overlay="0")

platform = client.create_platform_component(name = "SPI_EMIO_V0",hw_design = "$COMPONENT_LOCATION/../../design_1_wrapper_SPI_VO.xsa",os = "standalone",cpu = "ps7_cortexa9_0",domain_name = "standalone_ps7_cortexa9_0",generate_dtb = False,advanced_options = advanced_options,compiler = "gcc")

comp = client.create_app_component(name="SPI_Example_V0",platform = "$COMPONENT_LOCATION/../SPI_EMIO_V0/export/SPI_EMIO_V0/SPI_EMIO_V0.xpfm",domain = "standalone_ps7_cortexa9_0")

client.delete_component(name="SPI_EMIO_V0")

client.delete_component(name="SPI_EMIO_V0")

client.delete_component(name="SPI_Example_V0")

advanced_options = client.create_advanced_options_dict(dt_overlay="0")

platform = client.create_platform_component(name = "EMIO_SPI",hw_design = "$COMPONENT_LOCATION/../../design_1_wrapper_SPI_VO.xsa",os = "standalone",cpu = "ps7_cortexa9_0",domain_name = "standalone_ps7_cortexa9_0",generate_dtb = False,advanced_options = advanced_options,compiler = "gcc")

platform = client.get_component(name="EMIO_SPI")
status = platform.build()

comp = client.get_component(name="xspips_selftest_example")
comp.build()

status = platform.update_hw(hw_design = "$COMPONENT_LOCATION/../../design_1_wrapper_SPI_VO.xsa")

client.delete_component(name="xspips_eeprom_intr_example")

client.delete_component(name="xspips_selftest_example")

client.delete_component(name="componentName")

client.delete_component(name="xspips_eeprom_polled_example")

client.delete_component(name="componentName")

client.delete_component(name="xspips_eeprom_polled_example")

client.delete_component(name="xspips_eeprom_intr_example")

status = platform.build()

comp.build()

client.delete_component(name="xspips_selftest_example")

comp = client.create_app_component(name="SPI_Test_V0",platform = "$COMPONENT_LOCATION/../EMIO_SPI/export/EMIO_SPI/EMIO_SPI.xpfm",domain = "standalone_ps7_cortexa9_0")

status = platform.build()

comp = client.get_component(name="SPI_Test_V0")
comp.build()

status = platform.build()

comp.build()

advanced_options = client.create_advanced_options_dict(dt_overlay="0")

platform = client.create_platform_component(name = "EMIO_SPI_V1",hw_design = "$COMPONENT_LOCATION/../../design_1_wrapper_SPI_VO.xsa",os = "standalone",cpu = "ps7_cortexa9_0",domain_name = "standalone_ps7_cortexa9_0",generate_dtb = False,advanced_options = advanced_options,compiler = "gcc")

platform = client.get_component(name="EMIO_SPI_V1")
status = platform.update_hw(hw_design = "$COMPONENT_LOCATION/../../design_1_wrapper_SPI_VO.xsa")

status = platform.build()

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

status = platform.update_hw(hw_design = "$COMPONENT_LOCATION/../../design_1_wrapper_SPI_VO.xsa")

status = platform.build()

comp.build()

status = platform.build()

comp.build()

client.delete_component(name="xspips_slave_polled_example")

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

vitis.dispose()

