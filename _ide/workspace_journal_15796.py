# 2026-08-18T14:09:28.890243200
import vitis

client = vitis.create_client()
client.set_workspace(path="Vitis")

advanced_options = client.create_advanced_options_dict(dt_overlay="0")

platform = client.create_platform_component(name = "SPI_Test_Sean",hw_design = "$COMPONENT_LOCATION/../../design_1_wrapper.xsa",os = "standalone",cpu = "ps7_cortexa9_0",domain_name = "standalone_ps7_cortexa9_0",generate_dtb = False,advanced_options = advanced_options,compiler = "gcc")

comp = client.create_app_component(name="SPI_Sean_Test",platform = "$COMPONENT_LOCATION/../SPI_Test_Sean/export/SPI_Test_Sean/SPI_Test_Sean.xpfm",domain = "standalone_ps7_cortexa9_0")

platform = client.get_component(name="SPI_Test_Sean")
status = platform.build()

comp = client.get_component(name="SPI_Sean_Test")
comp.build()

status = platform.build()

comp.build()

status = platform.update_hw(hw_design = "$COMPONENT_LOCATION/../../design_1_wrapper.xsa")

status = platform.build()

client.delete_component(name="SPI_Test_Sean")

client.delete_component(name="componentName")

client.delete_component(name="SPI_Sean_Test")

client.delete_component(name="componentName")

advanced_options = client.create_advanced_options_dict(dt_overlay="0")

platform = client.create_platform_component(name = "SPI_Sean",hw_design = "$COMPONENT_LOCATION/../../design_1_wrapper.xsa",os = "standalone",cpu = "ps7_cortexa9_0",domain_name = "standalone_ps7_cortexa9_0",generate_dtb = False,advanced_options = advanced_options,compiler = "gcc")

comp = client.create_app_component(name="SPI_Seanw",platform = "$COMPONENT_LOCATION/../SPI_Sean/export/SPI_Sean/SPI_Sean.xpfm",domain = "standalone_ps7_cortexa9_0")

platform = client.get_component(name="SPI_Sean")
status = platform.build()

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

vitis.dispose()

