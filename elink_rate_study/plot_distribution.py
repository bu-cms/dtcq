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
from collections import defaultdict

mpl.rcParams['axes.prop_cycle'] = cycler("color", plt.get_cmap("tab20c").colors)

plt.style.use(hep.style.CMS)

newassignment=True

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

def load_data_from_tree(tree, config_file_name, NE=1):
    translate_dtc = lambda x : 11 + 10*((x-217)//7) + (x-217)%7
    tree_array = tree.arrays(["barrel", "disk", "layer", "dtc", "module_id", "detid", "module_index", "stream_size_chip_aurora", "raw_hits", "nchip"])
    data = []
    with open(config_file_name) as config:
        for module_index in range(3892):
            dtc_id = tree_array[b"dtc"][module_index]
            #if dtc_id > 100:
            #    dtc_id = translate_dtc(dtc_id)
            if dtc_id > 20:
                continue
            is_barrel = tree_array[b"barrel"][module_index]
            layer = tree_array[b"layer"][module_index]
            disk = tree_array[b"disk"][module_index]
            module_id = tree_array[b"module_id"][module_index] if is_barrel else module_index
            detid = tree_array[b"detid"][module_index]
            nchips = tree_array[b"nchip"][module_index]
            for chip_id in range(nchips):
                config.seek(0)
                FOUND = False
                for line in config:
                    words = line.replace("\n","").split("\t")
                    basename = words[0]
                    elinks = float(words[1])
                    module_cmssw_params = basename_to_module(basename)
                    (dtc_id_config, is_barrel_config, layer_config, disk_config, module_id_config, chip_id_config) = module_cmssw_params
                    if (dtc_id_config, is_barrel_config, layer_config, disk_config, module_id_config*is_barrel_config, chip_id_config) == (dtc_id, is_barrel, layer, disk, module_id*is_barrel, chip_id):
                        elinks = float(words[1])
                        layout = module_to_layout(module_cmssw_params)
                        basename = "dtc{}isBarrel{}layer{}disk{}module{}chip{}".format(dtc_id, is_barrel, layer, disk, module_index, chip_id)
                        FOUND = True
                        break
                if not FOUND:
                    print("cannot find matching layout for ", (dtc_id, is_barrel, layer, disk, module_id, chip_id))
                    continue
                raw_chip_data = tree_array[b"stream_size_chip_aurora"]
                raw_hits = tree_array[b"raw_hits"]
                # select the unpadded data for module and chip
                # if cabling map containing both IT and OT is used, dtc id can become 200 + original dtcid (which is small than 100). so we take a mod 100
                selector = (tree_array[b"module_index"]==module_index)
                if not chip_id < len(raw_chip_data[selector][0]):
                    print("for layout", (dtc_id, is_barrel, layer, disk, module_id), " out-of-bound chip_id: ", chip_id, " out of ", len(raw_chip_data[selector][0]))
                    continue
                raw_chip_data = raw_chip_data[selector][:,chip_id]
                raw_hits = raw_hits[selector][:,chip_id].flatten()
                raw_hits_mean = float(np.mean(raw_hits))
                if not type(raw_hits_mean) == float:
                    print(type(raw_hits_mean))
                raw_hits_std = float(np.std(raw_hits))
                if len(set(tree_array[b"module_index"][selector]))!=1:
                    print("matches for the following module is {}".format(len(set(tree_array[b"module_index"][selector]))))
                    print(line)
                    continue
                assert len(set(tree_array[b"module_index"][selector]))==1 # make sure we are selecting one module
                raw_size_std = np.std(raw_chip_data)
                # concatenate adjacent NE events
                nevents = len(raw_chip_data) - (len(raw_chip_data)%NE)
                raw_chip_data = np.reshape(raw_chip_data[:nevents], (-1, NE))
                raw_chip_data = np.sum(raw_chip_data, axis=1)
                padding = 64 - raw_chip_data%64
                padding[padding<6] += 64 # need to identify end of event by at least 6 continuous zeroes
                pad_chip_data = raw_chip_data + padding
                pad_data_std = np.std(raw_chip_data)/np.square(NE) # scale the std to event level (rather than every NE events)
                raw_evtsize = np.sum(raw_chip_data)/nevents
                pad_evtsize = np.sum(pad_chip_data)/nevents
                raw_occupancy = raw_evtsize * 0.75 / (1.28 * elinks * 1000)
                pad_occupancy = pad_evtsize * 0.75 / (1.28 * elinks * 1000)
                data.append({
                    "basename" : basename,
                    "detid" : detid,
                    "layout": layout,
                    "share" : elinks,
                    "raw_hits" : raw_hits_mean,
                    "raw_hits_std" : raw_hits_std,
                    "raw_size" : raw_evtsize,
                    "raw_size_std" : raw_size_std,
                    "pad_size" : pad_evtsize,
                    "pad_size_std" : pad_data_std,
                    "occupancy" : 100*raw_occupancy,
                    "padded_occupancy" : 100*pad_occupancy,
                    })
    return data

def sort_data_to_elinks(data):
    # first figure out how to assign chips to e-links
    basename_to_nchips_nlinks_map = defaultdict(lambda : [0,0])
    chip_to_elink_map = {}
    for chip_entry in data:
        basename_no_chip, chip_ID = chip_entry["basename"].split("chip")
        nelinks = float(chip_entry["share"])
        basename_to_nchips_nlinks_map[basename_no_chip][0] += 1
        basename_to_nchips_nlinks_map[basename_no_chip][1] += nelinks
    for basename, (nchips, nlinks) in basename_to_nchips_nlinks_map.items():
        if not int(nlinks)==nlinks:
            print(nlinks)
            exit()
        nlinks = int(nlinks)
        if nchips==2:
            if nlinks==6:
                chip_to_elink_map[basename] = {0:0, 1:3}
            elif nlinks==3:
                chip_to_elink_map[basename] = {0:0, 1:1}
            elif nlinks==2:
                chip_to_elink_map[basename] = {0:0, 1:1}
            elif nlinks==1: 
                raise ValueError("not expecting nchips={} elinks={}".format(nchips, nlinks))
            else:
                raise ValueError("not expecting nchips={} elinks={}".format(nchips, nlinks))
        else:
            assert nchips==4
            # Note, in case of 4 chips, chip ID assigned as:
            # Chip 0 , Chip 2
            # Chip 1 , Chip 3
            # where right means larger col number == higher rate
            if nlinks==4:
                chip_to_elink_map[basename] = {0:0, 1:1, 2:2, 3:3}
            elif nlinks==3:
                chip_to_elink_map[basename] = {0:0, 1:0, 2:1, 3:2}
            elif nlinks==2:
                if newassignment:
                    chip_to_elink_map[basename] = {0:0, 1:1, 2:0, 3:1}
                else:
                    chip_to_elink_map[basename] = {0:0, 1:0, 2:1, 3:1}
            elif nlinks==1:
                chip_to_elink_map[basename] = {0:0, 1:0, 2:0, 3:0}
            else:
                raise ValueError("not expecting nchips={} elinks={}".format(nchips, nlinks))
    # then do the assignment, especially merge chips for the same e-link
    finished = []
    unmerged = {}
    for chip_entry in data:
        basename_no_chip, chip_ID = chip_entry["basename"].split("chip")
        chip_ID = int(chip_ID)
        if chip_entry["share"].is_integer():
            nelinks = int(chip_entry["share"])
            for ielink in range(nelinks):
                elink_ID = chip_to_elink_map[basename_no_chip][chip_ID] + ielink
                elink_entry = {
                        "basename" : basename_no_chip + "elink{}".format(elink_ID),
                        "detid" : chip_entry["detid"],
                        "elink_id" : elink_ID,
                        "layout" : chip_entry["layout"],
                        "share" : 1,
                        "raw_hits" : chip_entry["raw_hits"]/nelinks,
                        "raw_hits_std" : chip_entry["raw_hits_std"]/nelinks,
                        "raw_size" : chip_entry["raw_size"]/nelinks,
                        "raw_size_std" : chip_entry["raw_size_std"]/(nelinks),
                        "pad_size" : chip_entry["pad_size"]/nelinks,
                        "pad_size_std" : chip_entry["pad_size_std"]/(nelinks),
                        "occupancy" : chip_entry["occupancy"],
                        "padded_occupancy" : chip_entry["padded_occupancy"],
                        }
                finished.append(elink_entry)
        else:
            elink_share = chip_entry["share"]
            elink_ID = chip_to_elink_map[basename_no_chip][chip_ID]
            elink_basename = basename_no_chip + "elink{}".format(elink_ID)
            if not elink_basename in unmerged:
                elink_entry = {
                        "basename" : basename_no_chip + "elink{}".format(elink_ID),
                        "detid" : chip_entry["detid"],
                        "elink_id" : elink_ID,
                        "layout" : chip_entry["layout"],
                        "share" : elink_share,
                        "raw_hits" : chip_entry["raw_hits"],
                        "raw_hits_std" : chip_entry["raw_hits_std"]*np.sqrt(elink_share),
                        "raw_size" : chip_entry["raw_size"],
                        "raw_size_std" : chip_entry["raw_size_std"]*np.sqrt(elink_share),
                        "pad_size" : chip_entry["pad_size"],
                        "pad_size_std" : chip_entry["pad_size_std"]*np.sqrt(elink_share),
                        "occupancy" : chip_entry["occupancy"]*elink_share,
                        "padded_occupancy" : chip_entry["padded_occupancy"]*elink_share,
                        }
                unmerged[elink_basename] = elink_entry
            else:
                unmerged[elink_basename]["share"] += elink_share
                unmerged[elink_basename]["raw_hits"] += chip_entry["raw_hits"]
                unmerged[elink_basename]["raw_hits_std"] += chip_entry["raw_hits"]*np.sqrt(elink_share)
                unmerged[elink_basename]["raw_size"] += chip_entry["raw_size"]
                unmerged[elink_basename]["raw_size_std"] += chip_entry["raw_size_std"]*np.sqrt(elink_share)
                unmerged[elink_basename]["occupancy"] += chip_entry["occupancy"]*elink_share
                unmerged[elink_basename]["pad_size"] += chip_entry["pad_size"]
                unmerged[elink_basename]["pad_size_std"] += chip_entry["pad_size_std"]*np.sqrt(elink_share)
                unmerged[elink_basename]["padded_occupancy"] += chip_entry["padded_occupancy"]*elink_share
            # check if ready to merge
            if abs(unmerged[elink_basename]["share"]-1)<0.1:
                finished.append(unmerged.pop(elink_basename))
    # after loop, check if any unmerged chip remaining
    for i in finished:
        if i["occupancy"]>95:
            print(i)
    assert len(unmerged)==0
    # sort the data by basename
    finished = list(sorted(finished, key=lambda x:(x["detid"], x["elink_id"])))
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

def add_lumi(data_in):
    for entry in data_in:
        if entry["layout"][0] == "TEPX" and entry["layout"][2]==1:
            for key in entry.keys():
                if "occupancy" in key:
                    print(f"{key} : {entry[key]}")
                    entry[key] = entry[key]*1.1
                    print(f"{key} : {entry[key]}")
    return data_in

def organize_data(data_in, distribution):
    def get_distribution(arr, dist):
        if "_relative_std" in dist:
            numerator = distribution.replace("_relative", "")
            denominator = distribution.replace("_relative_std", "")
            return arr[numerator]/arr[denominator]
        else:
            return arr[dist]
    data_out = {
            "all" : np.array([get_distribution(i, distribution) for i in data_in]),
            }
    # group by dtc
    for dtc in dtc_names:
        data_out[dtc] = np.array([get_distribution(i, distribution) for i in data_in if dtc in i["basename"]])
    # group by detector sections
    for section_name,layout in detector_sections.items():
        data_out[section_name] = np.array([get_distribution(i, distribution) for i in data_in if are_consistent_layout(layout, i["layout"])])
    return data_out

def plot_distribution(data, distribution, args, labels=["all"], stack=True, config_tag="default", version_tag="default", plot_tag="all", ylabel=None, ne=1, nopad=False):
    # parse additional args
    stack = args.stack
    ne = args.ne
    nopad = args.nopad
    fig,ax = plt.subplots(1,1)
    print("Plotting: distribution={}, labels={}, stack={}".format(distribution, labels, stack))
    # apply unsimulated effects
    if not args.no_unsim_effects:
        plt.text(1.0, 1.0, 'with un-simulated effects(10-20%)', horizontalalignment='right', verticalalignment='bottom', transform=ax.transAxes)
        unsim_effect = lambda x : 1.2 if "TEPX" in x else 1.1
        x = [data[label]*unsim_effect(label) for label in labels]
    else:
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
    if "relative_std" in distribution:
        ax.set_xlabel("relative std")
    elif "occupancy" in distribution:
        ax.set_xlabel("e-link occupancy (%)")
        ax.axvline(x=75, color="orange", lw=5, ls="--")
        ax.axvline(x=100, color="red",   lw=5, ls="--")
        # temporary
        #ax.set_xlim(40,110)
        #ax.set_ylim(0,400)
    elif "size" in distribution:
        ax.set_xlabel("chip data size per event (bit)")
    elif distribution == "ratio":
        ax.set_xlabel("compression ratio")
    elif distribution == "raw_hits":
        ax.set_xlabel("pixel hits")
    else:
        ax.set_xlabel(distribution)
    ax.legend()
    outdir = "plots/{}/{}".format(config_tag, version_tag)
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
    parser.add_argument("--config", type=str, default="../config/v8.config", help="config file as input.")
    parser.add_argument("--area", type=str, default="all", help="area of the detector to plot. Can be all, bydtc, bysection, TBPX_L1 etc.")
    parser.add_argument("--ne", type=int, default=1, help="number of events to be packed into the same stream, reduces padding needs. default=1.")
    parser.add_argument("--distribution", type=str, default="occupancy", help="distribution to plot, can be \"size\" or \"occupancy\".")
    parser.add_argument("--individual", action="store_true", help="also plot the individual plots. effective when area=bydtc or area=bysection.")
    parser.add_argument("--stack", action="store_true", help="wether we stack the hists or not.")
    parser.add_argument("--bychip", action="store_true", help="each chip gets an entry, compared to each entry for an elink.")
    parser.add_argument("--nopad", action="store_true", help="do not apply padding, NE argument ignored.")
    parser.add_argument("--lumi", action="store_true", help="add lumi trigger")
    parser.add_argument("--newload", action="store_true", help="don't use cache.")
    parser.add_argument("--no-unsim-effects", action="store_true", help="don't include unsimulated effects.")
    args = parser.parse_args()
    return args

def main():
    args = command_line()
    config_name = args.config
    config_tag = config_name.split("/")[-1]
    config_tag = config_tag.split(".")[0]
    if newassignment:
        config_tag += "_newassignment"
    #data = read_data_from_config(config_name)
    pkl_filename = args.ntuple.replace(".root", "{}_NE{}.pkl".format(config_tag,args.ne))
    version_tag = args.ntuple.split("/")[-2]
    data = None
    if os.path.exists(pkl_filename) and not args.newload:
        with open(pkl_filename, "rb") as f:
            print("loading data from file:"+pkl_filename+"...")
            data = pkl.load(f)
    else:
        with uproot.open(args.ntuple) as f:
            t = f["t"]
            print("loading data from file:"+args.ntuple+"...")
            data = load_data_from_tree(t, config_name, args.ne)
        with open(pkl_filename, "wb") as f:
            pkl.dump(data, f)
            print("pkl dumped at ", pkl_filename)
    if args.bychip:
        ylabel = "N(chips)"
    else:
        data = sort_data_to_elinks(data)
        pkl_elink_filename = args.ntuple.replace(".root", "{}_elinks_NE{}.pkl".format(config_tag, args.ne))
        with open(pkl_elink_filename,"wb") as f:
            pkl.dump(data, f)
        ylabel = "N(elinks)"
    if not args.nopad and args.distribution in ["occupancy"]:
        args.distribution = "padded_"+args.distribution
    if args.lumi:
        data = add_lumi(data)
        config_tag += "_lumi"
    data = organize_data(data,args.distribution)
    if args.bychip:
        config_tag += "_bychip"
    distribution = args.distribution
    # make the plots
    if args.area=="all":
        plot_distribution(data, distribution, args, labels=["all"], config_tag=config_tag, version_tag=version_tag, plot_tag="all", ylabel=ylabel)
    elif args.area=="bydtc":
        plot_distribution(data, distribution, args, labels=dtc_names, config_tag=config_tag, version_tag=version_tag, plot_tag="dtcs", ylabel=ylabel)
        if args.individual:
            for dtc_name in dtc_names:
                plot_distribution(data, distribution, args, labels=[dtc_name], config_tag=config_tag, version_tag=version_tag, plot_tag=dtc_name, ylabel=ylabel)
    elif args.area=="bysection":
        plot_distribution(data, distribution, args, labels=list(detector_sections.keys()), config_tag=config_tag, version_tag=version_tag, plot_tag="sections", ylabel=ylabel)
        if args.individual:
            for section_name in detector_sections.keys():
                plot_distribution(data, distribution, args, labels=[section_name], config_tag=config_tag, version_tag=version_tag, plot_tag=section_name.replace(" ", "_"), ylabel=ylabel)
    else:
        section_name = args.area.replace("_"," ")
        plot_distribution(data, distribution, args, labels=[section_name], config_tag=config_tag, version_tag=version_tag, plot_tag=section_name.replace(" ", "_"), ylabel=ylabel)

    return 0

if __name__ == "__main__":
    main()
