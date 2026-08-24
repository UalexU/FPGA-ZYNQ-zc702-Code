# 2026-08-12T14:46:55.802431700
import vitis

client = vitis.create_client()
client.set_workspace(path="Vitis")

platform = client.get_component(name="platform")
status = platform.build()

comp = client.get_component(name="hello_world")
comp.build()

comp = client.create_app_component(name="peripheral_tests",platform = "$COMPONENT_LOCATION/../platform/export/platform/platform.xpfm",domain = "standalone_ps7_cortexa9_0",template = "peripheral_tests")

status = platform.build()

comp = client.get_component(name="peripheral_tests")
comp.build()

