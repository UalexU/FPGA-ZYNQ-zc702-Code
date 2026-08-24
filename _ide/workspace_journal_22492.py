# 2026-08-19T13:58:41.271958800
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

status = comp.clean()

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

advanced_options = client.create_advanced_options_dict(dt_overlay="0")

platform = client.create_platform_component(name = "SPI_Sensor",hw_design = "$COMPONENT_LOCATION/../../design_1_wrapper.xsa",os = "standalone",cpu = "ps7_cortexa9_0",domain_name = "standalone_ps7_cortexa9_0",generate_dtb = False,advanced_options = advanced_options,compiler = "gcc")

platform = client.get_component(name="SPI_Sensor")
status = platform.build()

comp.build()

vitis.dispose()

