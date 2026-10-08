# SPI PIN ASSIGNMENT 
set_property PACKAGE_PIN W18  [get_ports spi_sclk_0]
set_property PACKAGE_PIN W19  [get_ports spi_mosi_0]
set_property PACKAGE_PIN AC18 [get_ports spi_miso_0]
set_property PACKAGE_PIN AC19 [get_ports {spi_ss_0[0]}]

set_property PACKAGE_PIN W18  [get_ports spi_sclk_1]
set_property PACKAGE_PIN W19  [get_ports spi_mosi_1]
set_property PACKAGE_PIN AC18 [get_ports spi_miso_1]
set_property PACKAGE_PIN AC19 [get_ports {spi_ss_1[0]}]


# I2C PIN ASSIGNMENT 
set_property PACKAGE_PIN W20 [get_ports iic_rtl_0_scl_io]
set_property PACKAGE_PIN Y20 [get_ports iic_rtl_0_sda_io]


# I/O Voltage Standards
set_property IOSTANDARD LVCMOS33 [get_ports spi_sclk_0]
set_property IOSTANDARD LVCMOS33 [get_ports spi_mosi_0]
set_property IOSTANDARD LVCMOS33 [get_ports spi_miso_0]
set_property IOSTANDARD LVCMOS33 [get_ports {spi_ss_0[0]}]

set_property IOSTANDARD LVCMOS33 [get_ports {spi_sclk_1 spi_mosi_1 spi_miso_1 spi_ss_1[0]} ]
set_property IOSTANDARD LVCMOS33 [get_ports {iic_rtl_0_scl_io iic_rtl_0_sda_io}]

# PULL UP RESISTORS
set_property PULLUP true [get_ports {iic_rtl_0_scl_io}]
set_property PULLUP true [get_ports {iic_rtl_0_sda_io}]
