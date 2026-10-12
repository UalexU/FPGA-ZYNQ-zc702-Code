# SPI 0 -> J25 (bank 13)
set_property PACKAGE_PIN W18  [get_ports spi_sclk_0]
set_property PACKAGE_PIN W19  [get_ports spi_mosi_0]
set_property PACKAGE_PIN AC18 [get_ports spi_miso_0]
set_property PACKAGE_PIN AC19 [get_ports {spi_ss_0[0]}]

# SPI 1 -> J26 (bank 12)
# set_property PACKAGE_PIN AC13 [get_ports spi_sclk_1]      ;# B12_L12_P
# set_property PACKAGE_PIN AD13 [get_ports spi_mosi_1]      ;# B12_L12_N
# set_property PACKAGE_PIN AE13 [get_ports spi_miso_1]      ;# B12_L10_P
# set_property PACKAGE_PIN AF13 [get_ports {spi_ss_1[0]}]   ;# B12_L10_N

# SPI 1 -> J23 (bank 12)
set_property PACKAGE_PIN AC16 [get_ports {spi_ss_1[0]}]      ;# J23-8  B12_L21_N
set_property PACKAGE_PIN AA17 [get_ports spi_mosi_1]      ;# J23-10 B12_L19_N
set_property PACKAGE_PIN AE15 [get_ports spi_miso_1]   ;# J23-12 B12_L17_N
set_property PACKAGE_PIN AD15 [get_ports spi_sclk_1]      ;# J23-14 B12_L15_N

# I2C -> J25 (bank 13)
set_property PACKAGE_PIN W20 [get_ports iic_rtl_0_scl_io]
set_property PACKAGE_PIN Y20 [get_ports iic_rtl_0_sda_io]

# I/O standards (banks 12 and 13 both on VCC_HR_B = 3.3 V)
set_property IOSTANDARD LVCMOS33 [get_ports {spi_sclk_0 spi_mosi_0 spi_miso_0 spi_ss_0[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {spi_sclk_1 spi_mosi_1 spi_miso_1 spi_ss_1[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {iic_rtl_0_scl_io iic_rtl_0_sda_io}]

# I2C pull-ups
set_property PULLUP true [get_ports {iic_rtl_0_scl_io iic_rtl_0_sda_io}]

