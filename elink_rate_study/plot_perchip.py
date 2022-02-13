#!/bin/env python

import numpy as np
import matplotlib as mpl
from matplotlib import pyplot as plt
import uproot
import mplhep as hep
import os,sys,re
import argparse
from cycler import cycler
import copy
import pickle as pkl

mpl.rcParams['axes.prop_cycle'] = cycler("color", plt.get_cmap("tab20c").colors)

plt.style.use(hep.style.CMS)

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

def read_data_from_config(config_file_name):
    data = []
    with open(config_file_name) as config:
        for line in config:
            words = line.replace("\n","").split("\t")
            basename = words[0]
            elinks = float(words[1])
            evtsize = float(words[-1])
            elinks_occupancy = evtsize * (64/1000) * 0.75 / (1.28 * elinks)
            layout = module_to_layout(basename_to_module(basename))
            data.append({
                "basename" : basename,
                "layout": layout,
                "share" : elinks,
                "size" : evtsize*64,
                "occupancy" : 100*elinks_occupancy,
                })
    return data

def dist_per_chip_from_tree(tree, config_file_name, fig, ax, NE=1):
    tree_array = tree.arrays(["barrel", "disk", "layer", "dtc", "module_id", "module_index", "stream_size_chip_aurora", "stream_size_chip_aurora_pad"])
    data = []
    with open(config_file_name) as config:
        for line in config:
            words = line.replace("\n","").split("\t")
            basename = words[0]
            elinks = float(words[1])
            #evtsize = float(words[-1])
            #elinks_occupancy = evtsize * (64/1000) * 0.75 / (1.28 * elinks)
            module_cmssw_params = basename_to_module(basename)
            (dtc_id, is_barrel, layer, disk, module_id, chip_id) = module_cmssw_params
            layout = module_to_layout(module_cmssw_params)
            raw_chip_data = tree_array[b"stream_size_chip_aurora"]
            # select the unpadded data for module and chip
            selector = (tree_array[b"dtc"]==dtc_id) & (tree_array[b"barrel"]==is_barrel) & (tree_array[b"layer"]==layer) & (tree_array[b"module_id"]==module_id) & (tree_array[b"disk"]==disk)
            raw_chip_data = raw_chip_data[selector][:,chip_id]
            assert len(set(tree_array[b"module_index"][selector]))==1 # make sure we are selecting one module
            relative_std = (np.std(raw_chip_data)/np.mean(raw_chip_data))
            # concatenate adjacent NE events
            nevents = len(raw_chip_data) - (len(raw_chip_data)%NE)
            raw_chip_data = np.reshape(raw_chip_data[:nevents], (-1, NE))
            raw_chip_data = np.sum(raw_chip_data, axis=1)
            padding = 64 - raw_chip_data%64
            padding[padding<6] += 64 # need to identify end of event by at least 6 continuous zeroes
            pad_chip_data = raw_chip_data + padding
            ax.clear()
            mean_raw = np.mean(raw_chip_data[:nevents])/NE
            std_raw = np.std(raw_chip_data[:nevents])/np.sqrt(NE)
            mean_pad = np.mean(pad_chip_data[:nevents])/NE
            std_pad = np.std(pad_chip_data[:nevents])/np.sqrt(NE)
            plt.hist(raw_chip_data, bins=50, label="no pad mean={:3.2f} std={:3.2f}".format(mean_raw, std_raw), histtype="step", color="orange", lw=4)
            plt.hist(pad_chip_data, bins=50, label="padded mean={:3.2f} std={:3.2f}".format(mean_pad, std_pad), histtype="step", color="blue", lw=4)
            ax.legend()
            fig.savefig("chip_dist/{}.png".format(basename))
            print("plot saved: chip_dist/{}.png".format(basename))

def sort_data_to_elinks(data):
    finished = []
    unmerged = {}
    for chip_entry in data:
        basename_no_chip, chip_ID = chip_entry["basename"].split("chip")
        chip_ID = int(chip_ID)
        if chip_entry["share"].is_integer():
            nelinks = int(chip_entry["share"])
            for ielink in range(nelinks):
                if nelinks == 1:
                    assert ielink == 0
                    if chip_entry["layout"][0]=="TEPX" and chip_entry["layout"][2]==1:
                        elink_ID = chip_ID - 1
                    else:
                        elink_ID = chip_ID
                elif nelinks == 2:
                    assert chip_entry["layout"][0]=="TFPX" and chip_entry["layout"][2]==1
                    elink_ID = 1 + ielink
                elif nelinks == 3:
                    assert chip_entry["layout"][0]=="TBPX" and chip_entry["layout"][1]==1
                    elink_ID = 3*chip_ID + ielink
                else:
                    raise Exception("nelinks out of range??")
                elink_entry = {
                        "basename" : basename_no_chip + "elink{}".format(elink_ID),
                        "layout" : chip_entry["layout"],
                        "share" : 1,
                        "relative_std" : chip_entry["relative_std"]*np.sqrt(nelinks),
                        "size" : chip_entry["size"]/nelinks,
                        "padded_size" : chip_entry["padded_size"]/nelinks,
                        "occupancy" : chip_entry["occupancy"],
                        "padded_occupancy" : chip_entry["padded_occupancy"],
                        }
                finished.append(elink_entry)
        else:
            elink_share = chip_entry["share"]
            if elink_share == 0.5:
                elink_ID = chip_ID // 2
            elif elink_share == 0.25:
                elink_ID = 0
            else:
                raise Exception("elink share invalid?? ({})".format(elink_share))
            elink_basename = basename_no_chip + "elink{}".format(elink_ID)
            if not elink_basename in unmerged:
                elink_entry = {
                        "basename" : basename_no_chip + "elink{}".format(elink_ID),
                        "layout" : chip_entry["layout"],
                        "share" : elink_share,
                        "relative_std" : chip_entry["relative_std"]*elink_share*np.sqrt(elink_share),
                        "size" : chip_entry["size"],
                        "padded_size" : chip_entry["padded_size"],
                        "occupancy" : chip_entry["occupancy"]*elink_share,
                        "padded_occupancy" : chip_entry["padded_occupancy"]*elink_share,
                        }
                unmerged[elink_basename] = elink_entry
            else:
                unmerged[elink_basename]["share"] += elink_share
                unmerged[elink_basename]["relative_std"] += chip_entry["relative_std"]*elink_share*np.sqrt(elink_share),
                unmerged[elink_basename]["size"] += chip_entry["size"]
                unmerged[elink_basename]["occupancy"] += chip_entry["occupancy"]*elink_share
                unmerged[elink_basename]["padded_size"] += chip_entry["padded_size"]
                unmerged[elink_basename]["padded_occupancy"] += chip_entry["padded_occupancy"]*elink_share
            # check if ready to merge
            if abs(unmerged[elink_basename]["share"]-1)<0.1:
                finished.append(unmerged.pop(elink_basename))
    # after loop, check if any unmerged chip remaining
    for i in finished:
        if i["occupancy"]>95:
            print(i)
    assert len(unmerged)==0
    return finished

def read_data_from_txt(input_file_name):
    data = []
    input_file = open(input_file_name)
    for line in input_file.readlines():
        line = line.replace("\n","")
        basename, share, size, occupancy = line.split("\t")
        layout = module_to_layout(basename_to_module(basename))
        data.append({
            "basename" : basename,
            "layout": layout,
            "share" : float(share),
            "size" : float(size),
            "occupancy" : 100*float(occupancy),
            })
    return data

dtc_names = ["dtc{}".format(idtc) for idtc in range(11,18)]
detector_sections = {
        "TBPX L1" : ("TBPX", 1, -1),
        "TBPX L2" : ("TBPX", 2, -1),
        "TBPX L3" : ("TBPX", 3, -1),
        "TBPX L4" : ("TBPX", 4, -1),
        "TFPX R1" : ("TFPX", -1, 1),
        "TFPX R2" : ("TFPX", -1, 2),
        "TFPX R3" : ("TFPX", -1, 3),
        "TFPX R4" : ("TFPX", -1, 4),
        "TEPX R1" : ("TEPX", -1, 1),
        "TEPX R2" : ("TEPX", -1, 2),
        "TEPX R3" : ("TEPX", -1, 3),
        "TEPX R4" : ("TEPX", -1, 4),
        "TEPX R5" : ("TEPX", -1, 5),
        }

# compare only non-negative part of 2 layouts
def are_consistent_layout(layout_1, layout_2):
    ret = layout_1[0]==layout_2[0]
    ret = ret and (layout_1[1]<0 or layout_2[1]<0 or layout_1[1]==layout_2[1])
    ret = ret and (layout_1[2]<0 or layout_2[2]<0 or layout_1[2]==layout_2[2])
    return ret

def organize_data(data_in, distribution):
    data_out = {
            "all" : np.array([i[distribution] for i in data_in]),
            }
    # group by dtc
    for dtc in dtc_names:
        data_out[dtc] = np.array([i[distribution] for i in data_in if dtc in i["basename"]])
    # group by detector sections
    for section_name,layout in detector_sections.items():
        data_out[section_name] = np.array([i[distribution] for i in data_in if are_consistent_layout(layout, i["layout"])])
    return data_out

def plot_distribution(data, distribution, labels=["all"], stack=True, config_tag="default", plot_tag="all", ylabel=None, ne=1, nopad=False):
    fig,ax = plt.subplots(1,1)
    print("Plotting: distribution={}, labels={}, stack={}".format(distribution, labels, stack))
    x = [data[label] for label in labels]
    if stack:
        ax.hist(x, stacked=True, label=labels)
    else:
        ax.hist(x, density=True, histtype="step", lw=3, alpha=0.8, label=labels)
    if stack:
        if ylabel == None:
            ylabel = "N(elinks)"
        ax.set_ylabel(ylabel)
    else:
        ax.set_ylabel("distribution")
    if "occupancy" in distribution:
        ax.set_xlabel("e-link occupancy (%)")
        ax.axvline(x=75, color="orange", lw=5, ls="--")
        ax.axvline(x=100, color="red",   lw=5, ls="--")
    elif "size" in distribution:
        ax.set_xlabel("chip data size per event (bit)")
    elif distribution == "ratio":
        ax.set_xlabel("compression ratio")
    ax.legend()
    outdir = "plots/{}".format(config_tag)
    if not os.path.exists(outdir):
        os.makedirs(outdir)
    if nopad:
        plot_tag += "_nopadding"
    else:
        plot_tag += "_NE"+str(ne)
    if stack:
        plot_tag = plot_tag + "_stack"
    plot_file_name = "{}/{}_{}.png".format(outdir, distribution, plot_tag)
    fig.savefig(plot_file_name)
    print("saved plots {}".format(plot_file_name))
    del ax,fig

def command_line():
    parser = argparse.ArgumentParser(prog="Plotter.")
    parser.add_argument("ntuple", type=str, help="root file that contains rate information.")
    parser.add_argument("--config", type=str, default="../config/default.config", help="config file as input.")
    parser.add_argument("--area", type=str, default="all", help="area of the detector to plot. Can be all, bydtc, bysection, TBPX_L1 etc.")
    parser.add_argument("--ne", type=int, default=1, help="number of events to be packed into the same stream, reduces padding needs. default=1.")
    parser.add_argument("--distribution", type=str, default="occupancy", help="distribution to plot, can be \"size\" or \"occupancy\".")
    parser.add_argument("--individual", action="store_true", help="also plot the individual plots. effective when area=bydtc or area=bysection.")
    parser.add_argument("--stack", action="store_true", help="wether we stack the hists or not.")
    parser.add_argument("--bychip", action="store_true", help="each chip gets an entry, compared to each entry for an elink.")
    parser.add_argument("--nopad", action="store_true", help="do not apply padding, NE argument ignored.")
    parser.add_argument("--newload", action="store_true", help="don't use cache.")
    args = parser.parse_args()
    return args

def main():
    args = command_line()
    config_name = args.config
    #data = read_data_from_config(config_name)
    pkl_filename = args.ntuple.replace(".root", "_NE{}.pkl".format(args.ne))
    fig, ax = plt.subplots(1);
    with uproot.open(args.ntuple) as f:
        t = f["t"]
        print("loading data from file:"+args.ntuple+"...")
        dist_per_chip_from_tree(t, config_name, fig, ax, args.ne)
    return 0

if __name__ == "__main__":
    main()
