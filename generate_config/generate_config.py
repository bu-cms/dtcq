#!/bin/env python

import os,sys
import argparse
import regex as re

class Chip:
    def __init__(self, file_name_str):
        self.file_name = file_name_str
        basename = file_name_str.split("/")[-1] #remove parent dirs
        basename = basename.split(".")[0] #remove filetype
        self.basename = basename
        pattern = re.compile("dtc(\d+)isBarrel(\d+)layer(\d+)disk(\d+)module(\d+)chip(\d+)")
        result = pattern.match(basename)
        if not result:
            raise ValueError("{} is not a valid input filename.".format(file_name_str))
        values = result.groups()
        self.dtc_id = int(values[0])
        self.is_barrel = True
        if int(values[1])==0: self.is_barrel = False
        self.layer = int(values[2])
        self.disk = int(values[3])
        self.module = int(values[4])
        self.chip_id = int(values[5])

def commandline():
    parser = argparse.ArgumentParser(prog='Plotter.')
    parser.add_argument('inpath', type=str, help='Input event-size logfiles to use.', nargs='*')
    parser.add_argument('--output', type=str, default="default.config", help='name for output file')
    parser.add_argument('--wiring-config', type=str, default="InnerTrackerDTCsToModules.txt", help='name for input wiring config file')
    args = parser.parse_args()
    return args

def load_elinks_nchips_mapping(config_file_name=None):
    section_naming_map = {
            "PXB" : "TBPX",
            "FPIX_1" : "TFPX",
            "FPIX_2" : "TEPX",
            }
    m = {}
    debug = {}
    with open(config_file_name) as ifstream:
        for line in ifstream.readlines():
            elements = line.replace("\n","").replace(" ","").split(",")
            if len(elements) == 1 : continue
            assert len(elements) == 18, f"{len(elements)}"
            DTC_ID = int(elements[2])
            N_ELinks = int(elements[7])
            IsLongBarrel = int(elements[10])
            Module_DetID = int(elements[11])
            Section = section_naming_map[elements[12]]
            Layer = int(elements[13])
            Ring = int(elements[14])
            NChips = int(elements[16])
            key = (Section, Layer, Ring)
            val = (N_ELinks, NChips)
            if not key in m.keys():
                m[key] = val
                debug[key] = {val: [Module_DetID]}
            else:
                #assert(val == m[key])
                if val in debug[key].keys():
                    debug[key][val].append(Module_DetID)
                else:
                    debug[key][val] = [Module_DetID]
        for key in debug.keys():
            if len(debug[key])>1:
                print("Warning: multiple configurations of modules for DTC={} IsBarrel={} Layer={} Ring={}".format(key[0],key[1],key[2],key[3]))
                print("ELinks & NChips configurations:")
                print(debug[key])
    return m

def load_chip_to_length_mapping(filenames):
    mapping = {}
    for filename in filenames:
        with open(filename) as ifstream:
            for line in ifstream.readlines():
                elements = line.replace("\n","").split("\t")
                assert(len(elements)==2)
                basename = elements[0]
                avgsize = float(elements[1])
                mapping[basename] = avgsize
    return mapping

def group_by_module(chip_basename_mapping):
    mapping = {}
    for basename, avgsize in chip_basename_mapping.items():
        chip = Chip(basename)
        module = (chip.dtc_id, chip.is_barrel, chip.layer, chip.disk, chip.module)
        if not module in mapping.keys():
            mapping[module] = {}
        mapping[module][basename] = avgsize
    return mapping

# Module name in CMSSW doesn't match the convention as in routing table
# Some mapping between them is needed
# This function maps the CMSSW coornitates (DTC_ID, IS_BARREL, LAYER, DISK, MODULE)
# to routing table coordinates (Section, Layer, Ring)
def module_to_layout(module):
    # unpack module info as in CMSSW
    dtc_id, is_barrel, layer, disk, module = module
    # convert module coorniates as defined in routing table
    # which depends on the location of the detector: TBPX/TEPX/TFPX
    if is_barrel: 
        Section = "TBPX"
        Layer = layer
        Ring = abs(module-5)+1
    elif disk > 8: 
        Section = "TEPX"
        Layer = disk - 8
        Ring = layer
    else:
        Section = "TFPX"
        Layer = disk
        Ring = layer
    layout = (Section, Layer, Ring)
    return layout

def rank_module_by_elinks(module_mapping, layout_to_elinks_nchips_mapping):
    module_list=list(module_mapping.keys())
    module_elinks = []
    module_nchips = []
    for module in module_list:
        layout = module_to_layout(module)
        elinks,nchips = layout_to_elinks_nchips_mapping[layout]
        module_elinks.append(elinks)
        module_nchips.append(nchips)
    return [module for _,_,module in sorted(zip(module_nchips,module_elinks,module_list), key=lambda x:-10*x[0]+x[1])]


def get_elinks_assignment(nelinks, nchips):
    assert (nelinks,nchips) in [(6,2), (2,2), (3,2), (2,4), (3,4), (1,4), (4,4)]
    if nelinks==3 and nchips==2:
        return [2,1]
    elif nelinks==3 and nchips==4:
        return [1,1,0.5,0.5]
    else:
        return [nelinks/nchips] * nchips
    #return [nelinks/nchips] * nchips

def generate_config(chip_to_length_mapping, layout_to_elinks_nchips_mapping, output_filename):
    chip_to_elinks_nevents_mapping = {}
    grouped_by_module = group_by_module(chip_to_length_mapping)
    ranked_modules = rank_module_by_elinks(grouped_by_module, layout_to_elinks_nchips_mapping)
    for module in ranked_modules:
        permodule_chip_to_length_mapping = grouped_by_module[module]
        layout = module_to_layout(module)
        try:
            elinks,nchips = layout_to_elinks_nchips_mapping[layout]
        except KeyError:
            print(layout)
            print(layout_to_elinks_nchips_mapping.keys())
            raise ValueError
        assert(nchips == len(permodule_chip_to_length_mapping))
        # rank chips by average event size
        ranked_keys = sorted(permodule_chip_to_length_mapping.keys(), key=permodule_chip_to_length_mapping.get, reverse=True)
        # assign elinks
        elinks_assignment = get_elinks_assignment(elinks, nchips)
        for i in range(nchips):
            basename = ranked_keys[i]
            evtsize = permodule_chip_to_length_mapping[basename]
            nevents = 1
            if evtsize < 30:
                nevents = nevents*2
            chip_to_elinks_nevents_mapping[ranked_keys[i]] = (elinks_assignment[i], nevents, evtsize)
    # write the mapping to output file
    with open(output_filename, "w") as ofstream:
        for basename, vals in chip_to_elinks_nevents_mapping.items():
            elinks_ratio = vals[0]
            nevents = vals[1]
            evtsize = vals[2]
            ofstream.write("{}\t{:3.2f}\t{}\t{}\n".format(basename, elinks_ratio, nevents, evtsize))
    print("generated config file: {}".format(output_filename))
    return

def main():
    args = commandline()
    if len(args.inpath) == 0:
        print("Warning: no input file provided, output config will be empty!")
    layout_to_elinks_nchips_mapping = load_elinks_nchips_mapping(args.wiring_config)
    chip_to_length_mapping = load_chip_to_length_mapping(args.inpath)
    generate_config(chip_to_length_mapping, layout_to_elinks_nchips_mapping, args.output)
    print("done")

if __name__ == "__main__":
    main()
