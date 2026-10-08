# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "C:\\FPGA\\TE0745\\fw\\ethernet_Test\\zynq_fsbl\\zynq_fsbl_bsp\\include\\diskio.h"
  "C:\\FPGA\\TE0745\\fw\\ethernet_Test\\zynq_fsbl\\zynq_fsbl_bsp\\include\\ff.h"
  "C:\\FPGA\\TE0745\\fw\\ethernet_Test\\zynq_fsbl\\zynq_fsbl_bsp\\include\\ffconf.h"
  "C:\\FPGA\\TE0745\\fw\\ethernet_Test\\zynq_fsbl\\zynq_fsbl_bsp\\include\\sleep.h"
  "C:\\FPGA\\TE0745\\fw\\ethernet_Test\\zynq_fsbl\\zynq_fsbl_bsp\\include\\xilffs.h"
  "C:\\FPGA\\TE0745\\fw\\ethernet_Test\\zynq_fsbl\\zynq_fsbl_bsp\\include\\xilffs_config.h"
  "C:\\FPGA\\TE0745\\fw\\ethernet_Test\\zynq_fsbl\\zynq_fsbl_bsp\\include\\xilrsa.h"
  "C:\\FPGA\\TE0745\\fw\\ethernet_Test\\zynq_fsbl\\zynq_fsbl_bsp\\include\\xiltimer.h"
  "C:\\FPGA\\TE0745\\fw\\ethernet_Test\\zynq_fsbl\\zynq_fsbl_bsp\\include\\xtimer_config.h"
  "C:\\FPGA\\TE0745\\fw\\ethernet_Test\\zynq_fsbl\\zynq_fsbl_bsp\\lib\\libxilffs.a"
  "C:\\FPGA\\TE0745\\fw\\ethernet_Test\\zynq_fsbl\\zynq_fsbl_bsp\\lib\\libxilrsa.a"
  "C:\\FPGA\\TE0745\\fw\\ethernet_Test\\zynq_fsbl\\zynq_fsbl_bsp\\lib\\libxiltimer.a"
  )
endif()
