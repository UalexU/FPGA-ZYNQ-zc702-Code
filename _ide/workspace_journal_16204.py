# 2026-08-20T11:19:32.753619400
import vitis

client = vitis.create_client()
client.set_workspace(path="Vitis")

comp = client.create_app_component(name="SPI_Sensor_Test_8bit_V1",platform = "$COMPONENT_LOCATION/../SPI_Sensor/export/SPI_Sensor/SPI_Sensor.xpfm",domain = "standalone_ps7_cortexa9_0")

comp = client.get_component(name="SPI_Sensor_Test_8bit_V1")
status = comp.import_files(from_loc="", files=["C:\Users\Admin\XilinxProjects\Test\Vitis\SPI_Sensor_Test_8bit\src\platform.c", "C:\Users\Admin\XilinxProjects\Test\Vitis\SPI_Sensor_Test_8bit\src\platform.h", "C:\Users\Admin\XilinxProjects\Test\Vitis\SPI_Sensor_Test_8bit\src\SPI_Sean_Test.c"])

platform = client.get_component(name="SPI_Sensor")
status = platform.build()

comp = client.get_component(name="SPI_Sensor_Test_8bit_V1")
comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp = client.get_component(name="SPI_Sensor_Test_8bit")
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

status = platform.build()

comp.build()

status = platform.build()

comp.build()

vitis.dispose()

