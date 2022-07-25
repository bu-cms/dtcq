#!/bin/env python
import os
import argparse
import re

#############
# This is the script that update on top of default.config with added e-links
# The current version number is:
VERSION = 8
# including 1 e-link addtion to the certain sections (for each version):
# v5 : TFPX R4, TEPX R1, TEPX R3 (as in v5.config)
# v7 : v5 + TFPX R2, TFPX R3, TEPX R2, TEPX R4
# v8 : v5 + TFPX R2, TEPX R4
# v8 is the currently decided version

def basename_to_module(basename):
    basename = basename.split("/")[-1] #remove parent dirs
    basename = basename.split(".")[0] #remove filetype
    pattern = re.compile("dtc(\d+)isBarrel(\d+)layer(\d+)disk(\d+)module(\d+)chip(\d+)")
    result = pattern.match(basename)
    if not result:
        raise ValueError("{} is not a valid basename.".format(basename))
    values = result.groups()
    dtc_id = int(values[0])
    is_barrel = True
    if int(values[1])==0: is_barrel = False
    layer = int(values[2])
    disk = int(values[3])
    module = int(values[4])
    chip_id = int(values[5])
    return (dtc_id, is_barrel, layer, disk, module, chip_id)


def module_to_layout(module):
    # unpack module info as in CMSSW
    dtc_id, is_barrel, layer, disk, module, chip_id = module
    # convert module coorniates as defined in routing table
    # which depends on the location of the detector: TBPX/TEPX/TFPX
    if is_barrel: 
        Section = "TBPX"
        Layer = layer
        Ring = abs(module-5)+1
    elif dtc_id%10 == 6 or dtc_id%10 == 7: 
        Section = "TEPX"
        Layer = disk - 8
        Ring = layer
    else:
        Section = "TFPX"
        Layer = disk
        Ring = layer
    layout = (Section, Layer, Ring)
    return layout

def commandline():
    parser = argparse.ArgumentParser(prog="formatter")
    parser.add_argument("--input", type=str, default="default.config")
    parser.add_argument("--output", type=str, default="v{}.config".format(VERSION))
    args = parser.parse_args()
    return args

def main():
    args = commandline()
    in_fn = args.input
    out_fn = args.output
    with open(in_fn) as input_file:
        with open(out_fn, "w") as output_file:
            for line in input_file:
                basename, share, ne, size = line.split("\t")
                Section, Layer, Ring = module_to_layout(basename_to_module(basename))
                if Section=="TEPX" and Ring==1:
                    share = "{:1.2f}".format(1.0)
                #if Section=="TEPX" and Ring==2 and ("chip2" in basename or "chip3" in basename):
                #    share = "{:1.2f}".format(1.0)
                if Section=="TEPX" and Ring==3:
                    share = "{:1.2f}".format(0.5)
                if Section=="TEPX" and Ring==4:
                    share = "{:1.2f}".format(0.5)
                if Section=="TFPX" and Ring==2 and "chip1" in basename:
                    share = "{:1.2f}".format(2.0)
                #if Section=="TFPX" and Ring==3 and ("chip2" in basename or "chip3" in basename):
                #    share = "{:1.2f}".format(1.0)
                if Section=="TFPX" and Ring==4 and float(share)==0.25:
                    share = "{:1.2f}".format(0.5)
                new_line = "\t".join([basename, share, ne, size])
                output_file.write(new_line)

if __name__ == "__main__":
    main()
