# 2026-08-13T09:56:11.994076900
import vitis

client = vitis.create_client()
client.set_workspace(path="Vitis")

client.sync_git_example_repo(name="vitis_hls_examples")

client.sync_git_example_repo(name="vitis_hls_examples")

comp = client.create_hls_component(name = "array_partition_block_cyclic",template = "vitis_hls_examples/Array/array_partition_block_cyclic")

comp = client.get_component(name="array_partition_block_cyclic")
comp.run(operation="SYNTHESIS")

client.delete_component(name="array_partition_block_cyclic")

