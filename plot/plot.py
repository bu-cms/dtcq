#!/bin/env python

from matplotlib import pyplot as plt
import numpy as np
import argparse
import os, sys
import regex as re
import pickle as pkl
from tqdm import tqdm

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

def read_nevents_from_dirname(data_dir):
    pattern = re.compile(".*_(\d+k?)evt.*")
    result = pattern.match(data_dir)
    if not result:
        raise ValueError("Error: input dir doesn't indicate number of events, should be in format like input_10kevt...")
    value = result.groups()[0]
    value = value.replace("k","000")
    return int(value)

def blocks(f, size=65535):
    while True:
        b = f.read(size)
        if not b: break
        yield b

def count_lines_in_file(f):
    return sum(b.count("\n") for b in blocks(f))


def load_data(data_dir, cache_file_name=""):
    # if there is cache file, load it
    if cache_file_name and os.path.exists(cache_file_name):
        try:
            with open(cache_file_name,'rb') as cache_file:
                data = pkl.load(cache_file)
                print("Loaded cheched data from file {}".format(cache_file_name))
                assert("h_event_size_perchip" in data.keys())
                return data
        except Exception:
            print("chached data invalid. Reload from original source...")
            raise Exception

    # otherwise we load data as normal
    data = {}

    # read the ordered input file names, so that we can match to the physical location of each chip
    with open("{}/ordered_file_names.txt".format(data_dir)) as ordered_file_names_txt:
        data['chip'] = [Chip(line.replace("\n","")) for line in ordered_file_names_txt.readlines()]
    nchips = len(data['chip'])

    # read the event length from input file
    nevents = read_nevents_from_dirname(data_dir)
    # A fine-binned histogram recording the distribution of event size, maximum=1000
    maxsize_perchip = 1000
    maxsize_total = int(nchips * maxsize_perchip / 2)
    data["h_event_size_perchip"] = np.zeros(shape=(nchips, maxsize_perchip), dtype=int)
    total_event_sizes = np.zeros(nevents, dtype=int)
    print("Reading event length...")
    for ichip in tqdm(range(nchips)):
        ievent = 0
        with open("../"+data["chip"][ichip].file_name, "rb") as chip_binary_input:
            byte = chip_binary_input.read(8)
            mask = (1 << 63)
            len_counting = 0
            while (byte):
                int_from_byte = int(byte[::-1].hex(),16)
                if ( (mask & int_from_byte) == mask ):
                    # new event
                    if len_counting == 0:
                        len_counting += 1
                    else:
                        while len_counting >= len(data["h_event_size_perchip"][ichip]):
                            # double the size of array if not enough
                            data["h_event_size_perchip"] = np.concatenate((data["h_event_size_perchip"], np.zeros_like(data["h_event_size_perchip"])), axis=1)
                        data["h_event_size_perchip"][ichip][len_counting]+=1
                        total_event_sizes[ievent] += len_counting
                        ievent += 1
                        len_counting = 1
                else:
                    len_counting += 1
                byte = chip_binary_input.read(8)
            if len_counting > 0:
                data["h_event_size_perchip"][ichip][len_counting]+=1
                total_event_sizes[ievent] += len_counting
    # histogramize the totals
    data["h_event_size_total"] = np.zeros(np.max(total_event_sizes)+1, dtype=int)
    for event_size in total_event_sizes:
        data["h_event_size_total"][event_size] += 1

    # read the buffer ocupancy from output file
    # store in histogram
    maxsize_perchip = 10000
    maxsize_total = int(nchips * maxsize_perchip / 2)
    data["h_buffer_size_perchip"] = np.zeros(shape=(nchips, maxsize_perchip),dtype=int)
    data["h_buffer_size_total"] = np.zeros(maxsize_total,dtype=int)
    input_fn = "{}/output_fifo_data_sizes.txt".format(data_dir)
    print("reading buffer occupancies...")
    with open(input_fn) as f:
        nlines = count_lines_in_file(f)
    with open(input_fn) as f:
        pbar = tqdm(total=nlines)
        for iline,line in enumerate(f.readlines()):
            pbar.update(1)
            numbers = line.replace("\n","").split(",")
            assert(nchips == len(numbers))
            total_buffer = 0
            for ichip in range(nchips):
                buffer_ichip = int(numbers[ichip])
                total_buffer += buffer_ichip
                while buffer_ichip >= len(data["h_buffer_size_perchip"][ichip]):
                    # double the size of array if not enough
                    data["h_buffer_size_perchip"] = np.concatenate((data["h_buffer_size_perchip"], np.zeros_like(data["h_buffer_size_perchip"])), axis=1)
                data["h_buffer_size_perchip"][ichip][buffer_ichip]+=1
            while total_buffer >= len(data["h_buffer_size_total"]):
                # double the size of array if not enough
                data["h_buffer_size_total"] = np.concatenate((data["h_buffer_size_total"], np.zeros_like(data["h_buffer_size_total"])), axis=0)
            data["h_buffer_size_total"][total_buffer] += 1
        pbar.close()

    # Save data into cache file
    if cache_file_name:
        with open(cache_file_name,'wb') as cache_file:
            pkl.dump(data, cache_file)
            print("Saved data into cache file {}".format(cache_file_name))

    return data

def find_local_maximum(a):
    ret = np.zeros(int(len(a)/2),dtype=np.int32)
    jentry = 0
    for ientry in range(1, len(a)-1):
        if (a[ientry] > a[ientry+1]) & (a[ientry] >= a[ientry-1]) & (a[ientry] > 1):
            ret[jentry] = a[ientry]
            jentry+=1
    ret.resize(jentry)
    return ret

def get_perchip_tag(data, ichip):
    perchip_tag = "DTC={}, ".format(data["chip"][ichip].dtc_id)
    if data["chip"][ichip].is_barrel: 
        perchip_tag += "Barrel"
    else: 
        perchip_tag += "Endcap/Forward"
    perchip_tag += "\n Layer={}".format(data["chip"][ichip].layer)
    perchip_tag += ", Disk={}".format(data["chip"][ichip].disk)
    perchip_tag += ", Module={}".format(data["chip"][ichip].module)
    perchip_tag += ", Chip_ID={}".format(data["chip"][ichip].chip_id)
    return perchip_tag

def is_sorted(iterable):
    for ientry in range(len(iterable)-1):
        if iterable[ientry] > iterable[ientry+1]:
            return False
    return True

def rebin_histogram(old_counts, new_bins, old_bins=None, input_density=True, output_density=True):
    # by default old_bins is a uniform binning with binwidth=1 and starting from 0
    if old_bins == None:
        old_bins = np.arange(len(old_counts)+1)
    assert(len(old_counts)+1 == len(old_bins))
    assert(is_sorted(old_bins))
    assert(is_sorted(new_bins))
    old_counts_copy = np.copy(old_counts)
    old_bin_width = np.array(old_bins[1:]) - np.array(old_bins[:-1])
    new_bin_width = np.array(new_bins[1:]) - np.array(new_bins[:-1])
    if input_density:
        old_counts_copy = old_counts_copy * old_bin_width
    new_counts = np.zeros(len(new_bins)-1)
    newbin_locator = 0
    for ioldbin in range(len(old_counts_copy)):
        old_low_edge = old_bins[ioldbin]
        old_high_edge = old_bins[ioldbin+1]
        if newbin_locator >= len(new_bins)-1:
            break
        while new_bins[newbin_locator+1] <= old_low_edge and newbin_locator<len(new_bins)-1:
            newbin_locator += 1
        while new_bins[newbin_locator] < old_high_edge and newbin_locator<len(new_bins)-1:
            new_counts[newbin_locator] += old_counts_copy[ioldbin] * (min(old_high_edge, new_bins[newbin_locator+1]) - max(old_low_edge, new_bins[newbin_locator])) / old_bin_width[ioldbin]
            newbin_locator += 1
    if output_density:
        new_counts = new_counts / np.sum(new_counts)
        new_counts = new_counts / new_bin_width
    return new_counts

def find_max_nonzero_bin(iterable):
    return np.max(iterable.nonzero()[-1])

def plot_buffer_distribution(data, outdir, args, data_to_compare=None, labels_for_comparison=("ver1", "ver2")):
    buffer_dist_outdir = "{}/buffer_distribution_plots".format(outdir)
    if args.no_event_length:
        buffer_dist_outdir += "_noEvtLength"
    if args.only_event_length:
        buffer_dist_outdir += "_onlyEvtLength"
    if args.no_cumulative:
        buffer_dist_outdir += "_noCumulative"
    if not os.path.exists(buffer_dist_outdir):
        os.makedirs(buffer_dist_outdir)

    nchips = len(data["chip"])

    # Setup binning
    max_perchip_buffer  = find_max_nonzero_bin(data["h_buffer_size_perchip"])
    max_perchip_evtsize = find_max_nonzero_bin(data[ "h_event_size_perchip"])
    binning_buffer  = np.arange(0,4+max_perchip_buffer,2)
    binning_evtsize = np.arange(0,4+max_perchip_evtsize,2)
    h_buffer_size_perchip = np.zeros(shape=(nchips, len(binning_buffer)-1))
    h_event_size_perchip  = np.zeros(shape=(nchips, len(binning_evtsize)-1))
    for ichip in range(nchips):
        h_buffer_size_perchip[ichip] = rebin_histogram(data["h_buffer_size_perchip"][ichip], binning_buffer)
        h_event_size_perchip[ichip]  = rebin_histogram(data[ "h_event_size_perchip"][ichip], binning_evtsize)

    fig, ax = plt.subplots()

    # Plot for individual fifos
    for ichip in range(nchips):
        if args.total_only:
            continue
        ax.clear()
        output_filename = "{}/bufferplot_ichip{}.png".format(outdir, data["chip"][ichip].basename)
        max_xlim = 300
        if not args.no_event_length:
            ax.plot(binning_evtsize[:-1]/16, h_event_size_perchip[ichip], label="event length")
            max_xlim = max(max_xlim, max(binning_evtsize))
        if not args.only_event_length:
            ax.plot(binning_buffer[:-1]/16 , h_buffer_size_perchip[ichip], label="fifo capacity")
            max_xlim = max(max_xlim, max(binning_buffer))
            # Plot cumulative
            if not args.no_cumulative:
                h_cumulative = np.zeros(len(h_buffer_size_perchip[ichip]),dtype=np.float32)
                h_normed = (binning_buffer[1:] - binning_buffer[:-1]) * h_buffer_size_perchip[ichip]
                for ibin in range(len(h_cumulative)):
                    h_cumulative[ibin] = sum(h_normed[ibin:])
                ax.plot(binning_buffer[:-1]/16, h_cumulative, label="comprehensive cumulative")
                max_xlim = max(max_xlim, max(binning_buffer))
        ax.legend()
        ax.set_xlim(0,max_xlim/16)
        ax.set_yscale('log')
        ax.set_xlabel("buffer occupancy (kb)")
        ax.set_ylabel("counts")
        perchip_tag = get_perchip_tag(data, ichip)
        ax.text(0.5, 0.7, perchip_tag, horizontalalignment='center', verticalalignment='center', transform=ax.transAxes)
        output_filename = "{}/{}.png".format(buffer_dist_outdir, data["chip"][ichip].basename)
        fig.savefig(output_filename)
        print("{} saved.".format(output_filename))
    
    # Plot for combination of fifos:
    # Setup binning
    max_total_buffer  = find_max_nonzero_bin(data["h_buffer_size_total"])
    max_total_evtsize = find_max_nonzero_bin(data["h_event_size_total" ])
    binning_buffer  = np.arange(0,40+max_total_buffer,20)
    binning_buffer  = np.arange(0,40+max_total_buffer,20)
    binning_evtsize = np.arange(0,40+max_total_evtsize,20)
    h_buffer_size_total = rebin_histogram(data["h_buffer_size_total"], binning_buffer)
    h_event_size_total  = rebin_histogram(data[ "h_event_size_total"], binning_evtsize)

    ax.clear()
    max_xlim = 300
    if not args.no_event_length:
        ax.plot(binning_evtsize[:-1]/16, h_event_size_total, label="event length")
        max_xlim = max(max_xlim, max(binning_evtsize))
    if not args.only_event_length:
        ax.plot(binning_buffer[:-1]/16 , h_buffer_size_total, label="fifo capacity")
        max_xlim = max(max_xlim, max(binning_buffer))
        # Plot cumulative
        if not args.no_cumulative:
            h_cumulative = np.zeros(len(h_buffer_size_total),dtype=np.float32)
            h_normed = (binning_buffer[1:] - binning_buffer[:-1]) * h_buffer_size_total
            for ibin in range(len(h_cumulative)):
                h_cumulative[ibin] = sum(h_normed[ibin:])
            ax.plot(binning_buffer[:-1]/16, h_cumulative, label="comprehensive cumulative")
            max_xlim = max(max_xlim, max(binning_buffer))
    ax.legend()
    ax.set_xlim(0,max_xlim/16)
    ax.set_yscale('log')
    ax.set_title("fifos combined")
    ax.set_xlabel("buffer capacity (kb)")
    ax.set_ylabel("density")
    output_filename = "{}/DTC11Total.png".format(buffer_dist_outdir)
    fig.savefig(output_filename)
    print("{} saved.".format(output_filename))
    return

def log_average_event_size(data, outdir):
    nchips = len(data["chip"])
    with open("{}/eventsize.log".format(outdir),"w") as ostream:
        for ichip in range(nchips):
            h_event_size_perchip = data["h_event_size_perchip"][ichip]
            avg_evt_size = np.sum(np.arange(len(h_event_size_perchip)) * h_event_size_perchip) / np.sum(h_event_size_perchip)
            ostream.write(data["chip"][ichip].basename)
            ostream.write("\t{:3.2f}\n".format(avg_evt_size))
    print("logged average event size in {}/eventsize.log".format(outdir))

def commandline():
    parser = argparse.ArgumentParser(prog='Plotter.')
    parser.add_argument('inpath', type=str, help='Input folder to use.')
    parser.add_argument('--no-event-length', action='store_true', help='In the buffer distribution plot, remove the line that shows the distribution of event length.')
    parser.add_argument('--only-event-length', action='store_true', help='In the buffer distribution plot, keep only the line that shows the distribution of event length.')
    parser.add_argument('--no-cumulative', action='store_true', help='Remove the line that shows the cumulative plot of buffer peak distribution.')
    parser.add_argument('--total-only', action='store_true', help='Plot only the plot for the sum of buffering between all fifos.')
    parser.add_argument('--log-average-event-size', action='store_true', help='generate a log file containing average event size for each chip')
    parser.add_argument('--tag', type=str, default="", help='Add a tag to be appeneded to the output dir name')
    parser.add_argument('--compare', type=str, default="", help='Compare to a different version, use the path that include the output plots. Comparison is done in buffer distribution plot only.')
    args = parser.parse_args()
    return args

def main():
    args = commandline()
    if args.compare:
        raise NotImplementedError
    while args.inpath[-1] == "/":
        args.inpath = args.inpath[:-1]
    outdir = "{}".format(args.inpath.split("/")[-1])
    if not os.path.exists(outdir):
        os.makedirs(outdir)
    cache_file_name = "{}/data.pkl".format(outdir)
    data = load_data(args.inpath, cache_file_name)
    if args.no_event_length and args.only_event_length:
        raise ValueError
    if args.log_average_event_size:
        log_average_event_size(data, outdir)
        return
    plot_buffer_distribution(data, outdir, args)
    return

if __name__ == "__main__":
    main()
