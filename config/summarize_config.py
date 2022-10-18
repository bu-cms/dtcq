#!/bin/env python
# Takes input from *.config file and outputs a summary of the e-link configuration

import sys
import os
import re
import argparse

# Define a CHIP class
class CHIP:
    def __init__(self, name, share=0):
        self.name = name
        self.share = share
        regex = re.compile(r'dtc(\d+)isBarrel(\d+)layer(\d+)disk(\d+)module(\d+)chip(\d+)')
        match = regex.match(name)
        self.dtc = int(match.group(1))
        self.isBarrel = int(match.group(2))
        self.layer = int(match.group(3))
        self.disk = int(match.group(4))
        self.module = int(match.group(5))
        self.chip = int(match.group(6))
        if self.isBarrel:
            self.region = "TBPX L" + str(self.layer)
        elif self.disk <= 8:
            self.region = "TFPX R" + str(self.layer)
        else:
            self.region = "TEPX R" + str(self.layer)
    def __str__(self):
        return self.name
    def print(self):
        print("Chip: " + self.name)
        print("  DTC: " + str(self.dtc))
        print("  isBarrel: " + str(self.isBarrel))
        print("  layer: " + str(self.layer))
        print("  disk: " + str(self.disk))
        print("  module: " + str(self.module))
        print("  chip: " + str(self.chip))
        print("  region: " + self.region)


# Parse command line arguments
def parse_args():
    parser = argparse.ArgumentParser(description='Summarize e-link configuration')
    parser.add_argument('config_file', type=str, help='Configuration file to parse')
    args = parser.parse_args()
    return args

def main():
    args = parse_args()
    config_file = args.config_file

    # Read configuration file
    with open(config_file, 'r') as f:
        print('Reading configuration file: ' + config_file)
        config = f.readlines()

    # Parse configuration file
    link_map = {}
    for line in config:
        basename, share, _, _ = line.split()
        c = CHIP(basename, share)
        if c.region not in link_map:
            link_map[c.region] = {}
        if c.disk not in link_map[c.region]:
            link_map[c.region][c.disk] = [0] * 4
        if link_map[c.region][c.disk][c.chip] == 0:
            link_map[c.region][c.disk][c.chip] = float(c.share)
        else:
            assert link_map[c.region][c.disk][c.chip] == float(c.share), "Inconsistent share for chip " + c.name
    
    # Print summary
    print('Summary of e-link configuration:')
    UIFORM_LINKS_WITHIN_REGION = True
    for region in sorted(link_map):
        print(region, " ".join([str(sum(link_map[region][disk])) for disk in sorted(link_map[region])]))
        if len(set([sum(link_map[region][disk]) for disk in sorted(link_map[region])])) > 1:
            UIFORM_LINKS_WITHIN_REGION = False
    if UIFORM_LINKS_WITHIN_REGION:
        print("Note: All disks within a region have the same number of links")
    else:
        print("Note: Not all disks within a region have the same number of links")

if __name__ == '__main__':
    main()