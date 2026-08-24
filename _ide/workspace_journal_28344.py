# 2026-08-14T10:14:49.988501900
import vitis

client = vitis.create_client()
client.set_workspace(path="Vitis")

platform = client.get_component(name="GPIO_Platform")
status = platform.build()

comp = client.get_component(name="hello_world")
comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

comp = client.clone_component(name="hello_world",new_name="LED_BTN_TEST_NoDrivers")

status = platform.build()

comp = client.get_component(name="LED_BTN_TEST_NoDrivers")
comp.build()

comp = client.clone_component(name="hello_world",new_name="Clock_Resets_SLRC")

status = platform.build()

comp = client.get_component(name="Clock_Resets_SLRC")
comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp = client.get_component(name="hello_world")
comp.build()

status = platform.build()

comp = client.get_component(name="Clock_Resets_SLRC")
comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp = client.get_component(name="hello_world")
comp.build()

status = platform.build()

comp = client.get_component(name="Clock_Resets_SLRC")
comp.build()

comp = client.clone_component(name="hello_world",new_name="Clock_Test")

client.delete_component(name="Clock_Resets_SLRC")

status = platform.build()

comp = client.get_component(name="Clock_Test")
comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp = client.get_component(name="hello_world")
comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

vitis.dispose()

