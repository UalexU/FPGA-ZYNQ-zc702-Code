# 2026-08-13T21:00:20.506881500
import vitis

client = vitis.create_client()
client.set_workspace(path="Vitis")

client.delete_component(name="hello_world")

client.delete_component(name="componentName")

client.delete_component(name="platform")

client.delete_component(name="platform")

client.delete_component(name="peripheral_tests")

client.delete_component(name="peripheral_tests")

advanced_options = client.create_advanced_options_dict(dt_overlay="0")

platform = client.create_platform_component(name = "GPIO_Platform",hw_design = "$COMPONENT_LOCATION/../../design_1_wrapper.xsa",os = "standalone",cpu = "ps7_cortexa9_0",domain_name = "standalone_ps7_cortexa9_0",generate_dtb = False,advanced_options = advanced_options,compiler = "gcc")

comp = client.create_app_component(name="hello_world",platform = "$COMPONENT_LOCATION/../GPIO_Platform/export/GPIO_Platform/GPIO_Platform.xpfm",domain = "standalone_ps7_cortexa9_0",template = "hello_world")

platform = client.get_component(name="GPIO_Platform")
status = platform.build()

comp = client.get_component(name="hello_world")
comp.build()

vitis.dispose()

