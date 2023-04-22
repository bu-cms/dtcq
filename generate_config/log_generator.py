import os
import uproot
import sys

inputfile = sys.argv[1]

with uproot.open(inputfile) as f:

    tree = f["t"]
    tree_array = tree.arrays(["barrel", "disk", "layer", "dtc", "module_id", "detid", "module_index", "stream_size_chip_aurora", "raw_hits", "nchip"])

    for module_index in range(3892):
        dtc_id = tree_array[b"dtc"][module_index]
        if dtc_id > 20:
            continue
        is_barrel = tree_array[b"barrel"][module_index]
        layer = tree_array[b"layer"][module_index]
        disk = tree_array[b"disk"][module_index]
        module_id = tree_array[b"module_id"][module_index]
        detid = tree_array[b"detid"][module_index]
        nchips = tree_array[b"nchip"][module_index]
        for chip_index in range(nchips) :
            print (f"dtc{dtc_id}isBarrel{int(is_barrel)}layer{layer}disk{disk}module{module_id}chip{chip_index}\t1.0")

