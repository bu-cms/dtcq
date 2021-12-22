#!/bin/env python
import argparse
from plot_distribution import module_to_layout,basename_to_module

class module:
    def __init__(self, dtc_id, section, layer, ring, module_ID):
        self.dtc_id = dtc_id
        self.section = section
        self.layer = layer
        self.ring = ring
        self.module_ID = module_ID
        self.elinks = 0
        self.rate = 0
        self.chips = {}
    def add_chip(self, chip_ID, rate, elinks):
        rate = rate
        self.chips[chip_ID] = {
                "rate" : rate,
                "elinks" : elinks,
                }
        self.elinks += elinks
        self.rate += rate
    def get_occupancy(self):
        return self.rate/(1.28*self.elinks)
    def generate_table_row(self):
        parameters = [str(i) for i in [self.module_ID, self.layer, self.ring, self.dtc_id]]
        for chip_ID in range(4):
            if chip_ID in self.chips:
                parameters.append("{:.3f}".format(self.chips[chip_ID]["rate"]))
            else:
                parameters.append("0.000")
        parameters.append("{:.3f}".format(self.rate))
        parameters.append(str(int(self.elinks)))
        parameters.append("{:.3f}".format(self.get_occupancy()))
        return "\t".join(parameters)


def load_modules(input_file_name):
    modules = {}
    input_file = open(input_file_name)
    for line in input_file.readlines():
        line = line.replace("\n","")
        basename, share, _, size = line.split("\t")
        cmssw_module_parameters = basename_to_module(basename)
        section, layer, ring = module_to_layout(cmssw_module_parameters)
        dtc_id, _, _, _, module_ID, chip_ID = cmssw_module_parameters
        key = (section, layer, ring, module_ID)
        if not key in modules:
            modules[key] = module(dtc_id, section, layer, ring, module_ID)
        modules[key].add_chip(chip_ID, float(size)*(64*0.75/1000), float(share))
    return modules

def command_line():
    parser = argparse.ArgumentParser(prog="Tabler")
    parser.add_argument("config", type=str, help="config file as input.")
    args = parser.parse_args()
    return args

def main():
    args = command_line()
    config_name = args.config
    modules = load_modules(config_name)
    for i,m in modules.items():
        print(m.generate_table_row())

if __name__ == "__main__":
    main()
