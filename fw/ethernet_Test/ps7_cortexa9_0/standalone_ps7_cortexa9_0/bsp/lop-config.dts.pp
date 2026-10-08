# 1 "C:/FPGA/TE0745/fw/ethernet_Test/ps7_cortexa9_0/standalone_ps7_cortexa9_0/bsp/lop-config.dts"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "C:/FPGA/TE0745/fw/ethernet_Test/ps7_cortexa9_0/standalone_ps7_cortexa9_0/bsp/lop-config.dts"

/dts-v1/;
/ {
        compatible = "system-device-tree-v1,lop";
        lops {
                lop_0 {
                        compatible = "system-device-tree-v1,lop,load";
                        load = "assists/baremetal_validate_comp_xlnx.py";
                };

                lop_1 {
                    compatible = "system-device-tree-v1,lop,assist-v1";
                    node = "/";
                    outdir = "C:/FPGA/TE0745/fw/ethernet_Test/ps7_cortexa9_0/standalone_ps7_cortexa9_0/bsp";
                    id = "module,baremetal_validate_comp_xlnx";
                    options = "ps7_cortexa9_0 C:/Xilinx2025.1/2025.1/Vitis/data/embeddedsw/ThirdParty/sw_services/lwip220_v1_2/src C:/FPGA/TE0745/fw/_ide/.wsdata/.repo.yaml";
                };

        };
    };
