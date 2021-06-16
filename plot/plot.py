#!/bin/env python

from matplotlib import pyplot as plt
import numpy as np
import argparse
import os, sys
import regex as re
import pickle as pkl

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
    pattern = re.compile(".*output_(\d+k?)evt.*")
    result = pattern.match(data_dir)
    if not result:
        raise ValueError("Error: input dir doesn't indicate number of events, should be in format like input_10kevt...")
    value = result.groups()[0]
    value = value.replace("k","000")
    return int(value)


def load_data(data_dir, cache_file_name=""):
    # if there is cache file, load it
    if cache_file_name and os.path.exists(cache_file_name):
        with open(cache_file_name,'rb') as cache_file:
            data = pkl.load(cache_file)
            print("Loaded cheched data from file {}".format(cache_file_name))
            return data

    # otherwise we load data as normal
    data = {}
    nchips = 60

    # read the ordered input file names, so that we can match to the physical location of each chip
    with open("{}/ordered_file_names.txt".format(data_dir)) as ordered_file_names_txt:
        data['chip'] = [Chip(line.replace("\n","")) for line in ordered_file_names_txt.readlines()]
    assert nchips == len(data['chip'])

    # read the event length from input file
    nevents = read_nevents_from_dirname(data_dir)
    data["event_size"] = np.zeros(shape=(nchips, nevents))
    print("Reading event length...")
    for ichip in range(nchips):
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
                        data["event_size"][ichip][ievent]=len_counting
                        ievent += 1
                        len_counting = 1
                else:
                    len_counting += 1
                byte = chip_binary_input.read(8)
            if len_counting > 0:
                data["event_size"][ichip][ievent]=len_counting

    # read the buffer ocupancy from output file
    # estimating nticks assuming maximum 1000 ticks per event
    nticks = 1000 * nevents
    data["buffer_over_time"] = np.zeros(shape=(nchips, nticks))
    input_fn = "{}/output_fifo_data_sizes.txt".format(data_dir)
    print("reading buffer occupancies...")
    with open(input_fn) as f:
        for iline,line in enumerate(f.readlines()):
            if iline >= nticks:
                nticks = nticks * 2
                data["buffer_over_time"].resize( (nchips, nticks) )
            numbers = line.replace("\n","").split(",")
            assert(nchips == len(numbers))
            #print(numbers)
            for ichip in range(nchips):
                #print(int(numbers[i]))
                data["buffer_over_time"][ichip][iline]=int(numbers[ichip])

    # Save data into cache file
    if cache_file_name:
        with open(cache_file_name,'wb') as cache_file:
            pkl.dump(data, cache_file)
            print("Saved data into cache file {}".format(cache_file_name))

    return data

def find_local_maximum(a):
    ret = np.zeros(int(len(a)/2))
    jentry = 0
    for ientry in range(1, len(a)-1):
        if (a[ientry] > a[ientry+1]) & (a[ientry] >= a[ientry-1]) & (a[ientry] > 1):
            ret[jentry] = a[ientry]
            jentry+=1
    ret.resize(jentry)
    return ret

def get_perchip_tag(data, ichip):
    perchip_tag = "DTC={}, ".format(data["chip"][ichip].basename)
    if data["chip"][ichip].is_barrel: 
        perchip_tag += "Barrel"
    else: 
        perchip_tag += "Endcap/Forward"
    perchip_tag += "\n Layer={}".format(data["chip"][ichip].layer)
    perchip_tag += ", Disk={}".format(data["chip"][ichip].disk)
    perchip_tag += ", Module={}".format(data["chip"][ichip].module)
    perchip_tag += ", Chip_ID={}".format(data["chip"][ichip].chip_id)
    return perchip_tag

def plot_buffer_over_time(data, outdir, args):
    buffer_over_time_outdir = "{}/buffer-over-time_plots".format(outdir)
    if not os.path.exists(buffer_over_time_outdir):
        os.makedirs(buffer_over_time_outdir)
    fig,ax = plt.subplots(1,1)
    total_buffer_over_time = np.zeros(len(data["buffer_over_time"][0]))
    # Plot for inidvidual FIFO
    for ichip,buffer_over_time in enumerate(data["buffer_over_time"]):
        total_buffer_over_time += buffer_over_time
        if args.total_only: continue
        x = np.arange(len(buffer_over_time))
        ax.plot(x,buffer_over_time*64/1024)
        plot_file_name = "{}/{}.png".format(buffer_over_time_outdir, data["chip"][ichip].basename)
        ax.set_xlabel("clock tick")
        ax.set_ylabel("buffer occupancy (kb)")
        perchip_tag = get_perchip_tag(data, ichip)
        ax.text(0.5, 0.8, perchip_tag, horizontalalignment='center', verticalalignment='center', transform=ax.transAxes)
        fig.savefig(plot_file_name)
        print("{} saved.".format(plot_file_name))
        ax.clear()
    # Plot for total 
    x = np.arange(len(total_buffer_over_time))
    ax.plot(x,total_buffer_over_time*64/1024)
    plot_file_name = "{}/DTC11Total.png".format(buffer_over_time_outdir)
    ax.set_xlabel("clock tick")
    ax.set_ylabel("buffer occupancy (kb)")
    ax.text(0.5, 0.8, "Sum of FIFO occupancies", horizontalalignment='center', verticalalignment='center', transform=ax.transAxes)
    fig.savefig(plot_file_name)
    print("{} saved.".format(plot_file_name))
    ax.clear()
    return

def plot_buffer_distribution(data, outdir, args, data_to_compare=None, labels_for_comparison=("ver1", "ver2")):
    buffer_dist_outdir = "{}/buffer_distribution_plots".format(outdir)
    if args.add_buffer_all_tick:
        buffer_dist_outdir += "_hasAllTick"
    if args.no_buffer_peak:
        buffer_dist_outdir += "_noPeakDist"
    if args.no_event_length:
        buffer_dist_outdir += "_noEvtLength"
    if args.cumulative_use_all:
        buffer_dist_outdir += "_cumulativeAllTick"
    if args.no_cumulative:
        buffer_dist_outdir += "_noCumulative"
    if not os.path.exists(buffer_dist_outdir):
        os.makedirs(buffer_dist_outdir)
    fig, ax = plt.subplots()

    # Plot for individual fifos
    max_single_chip_buffer = np.max(data["buffer_over_time"])
    max_single_chip_evtsize = np.max(data["event_size"])
    binning_buffer  = np.arange(0,max_single_chip_buffer,2)
    binning_evtsize = np.arange(0,max_single_chip_evtsize,2)
    max_xlim = 300
    for ichip in range(len(data["chip"])):
        if args.total_only:
            continue
        ax.clear()
        output_filename = "{}/bufferplot_ichip{}.png".format(outdir, data["chip"][ichip].basename)
        h_event_length,_ = np.histogram(data["event_size"][ichip], bins=binning_evtsize, density=True)
        if not args.no_event_length:
            ax.plot(binning_evtsize[:-1]//16, h_event_length, label="event length")
            max_xlim = max(max_xlim, 1.05*max(binning_evtsize))
        h_buffering_size,_ = np.histogram(data["buffer_over_time"][ichip], bins=binning_buffer, density=True)
        if args.add_buffer_all_tick:
            ax.plot(binning_buffer[:-1]//16 , h_buffering_size, label="fifo capacity")
            max_xlim = max(max_xlim, 1.05*max(binning_buffer))
        h_buffering_local_maximum,_ = np.histogram(find_local_maximum(data["buffer_over_time"][ichip]), bins=binning_buffer, density=True)
        if not args.no_buffer_peak:
            ax.plot(binning_buffer[:-1]//16, h_buffering_local_maximum, label="fifo capacity at local maximum")
            max_xlim = max(max_xlim, 1.05*max(binning_buffer))
        # Plot cumulative
        if not args.no_cumulative:
            h_cumulative = np.zeros(len(h_buffering_size))
            if args.cumulative_use_all:
                h_normed = (binning_buffer[1:] - binning_buffer[:-1]) * h_buffering_size
            else:
                h_normed = (binning_buffer[1:] - binning_buffer[:-1]) * h_buffering_local_maximum
            for ibin in range(len(h_cumulative)):
                h_cumulative[ibin] = sum(h_normed[ibin:])
            ax.plot(binning_buffer[:-1]//16, h_cumulative, label="comprehensive cumulative")
            max_xlim = max(max_xlim, 1.05*max(binning_buffer))
        ax.legend()
        ax.set_xlim(0,max_xlim//16)
        ax.set_yscale('log')
        ax.set_xlabel("buffer occupancy (kb)")
        ax.set_ylabel("counts")
        perchip_tag = get_perchip_tag(data, ichip)
        ax.text(0.5, 0.8, perchip_tag, horizontalalignment='center', verticalalignment='center', transform=ax.transAxes)
        output_filename = "{}/{}.png".format(buffer_dist_outdir, data["chip"][ichip].basename)
        fig.savefig(output_filename)
        print("{} saved.".format(output_filename))
    
    # Plot for combination of fifos:
    fifo_sizes_combined = np.zeros(len(data["buffer_over_time"][0]))
    event_length_combined = np.zeros(len(data["event_size"][0]))
    for ichip in range(len(data["buffer_over_time"])):
        fifo_sizes_combined += np.array(data["buffer_over_time"][ichip])
        event_length_combined += np.array(data["event_size"][ichip])
        #print("event length ichip {}:".format(ichip))
        #print(event_length[ichip])
        #print("event length combined:")
        #print(event_length_combined)
    ax.clear()
    binning_buffer  = np.arange(0,max(fifo_sizes_combined),5)
    binning_evtsize = np.arange(0,max(event_length_combined),5)
    h_event_length,_ = np.histogram(event_length_combined, bins=binning_evtsize, density=True)
    if not args.no_event_length:
        ax.plot(binning_evtsize[:-1]//16, h_event_length, label="event length")
        max_xlim = max(max_xlim, 1.05*max(binning_evtsize))
    h_buffering_size,_ = np.histogram(fifo_sizes_combined, bins=binning_buffer, density=True)
    if args.add_buffer_all_tick:
        ax.plot(binning_buffer[:-1]//16 , h_buffering_size, label="fifo capacity")
        max_xlim = max(max_xlim, 1.05*max(binning_buffer))
    h_buffering_local_maximum,_ = np.histogram(find_local_maximum(fifo_sizes_combined), bins=binning_buffer, density=True)
    if not args.no_buffer_peak:
        ax.plot(binning_buffer[:-1]//16, h_buffering_local_maximum, label="fifo capacity at local maximum")
        max_xlim = max(max_xlim, 1.05*max(binning_buffer))
    # Plot cumulative
    if not args.no_cumulative:
        h_cumulative = np.zeros(len(h_buffering_size))
        if args.cumulative_use_all:
            h_normed = (binning_buffer[1:] - binning_buffer[:-1]) * h_buffering_size
        else:
            h_normed = (binning_buffer[1:] - binning_buffer[:-1]) * h_buffering_local_maximum
        for ibin in range(len(h_cumulative)):
            h_cumulative[ibin] = sum(h_normed[ibin:])
        ax.plot(binning_buffer[:-1]//16, h_cumulative, label="comprehensive cumulative")
        max_xlim = max(max_xlim, 1.05*max(binning_buffer))
    ax.legend()
    ax.set_xlim(0,max_xlim//16)
    ax.set_yscale('log')
    ax.set_title("fifos combined")
    ax.set_xlabel("buffer capacity (kb)")
    ax.set_ylabel("density")
    output_filename = "{}/DTC11Total.png".format(buffer_dist_outdir)
    fig.savefig(output_filename)
    print("{} saved.".format(output_filename))
    return

def commandline():
    parser = argparse.ArgumentParser(prog='Plotter.')
    parser.add_argument('inpath', type=str, help='Input folder to use.')
    parser.add_argument('--add-buffer-all-tick', action='store_true', help='In the buffer distribution plot, add the line that include buffer size at all ticks (including trivial cases)')
    parser.add_argument('--no-buffer-peak', action='store_true', help='In the buffer distribution plot, remove the line that include only buffer size at peaks')
    parser.add_argument('--no-event-length', action='store_true', help='In the buffer distribution plot, remove the line that shows the distribution of event length.')
    parser.add_argument('--no-cumulative', action='store_true', help='Remove the line that shows the cumulative plot of buffer peak distribution.')
    parser.add_argument('--cumulative-use-all', action='store_true', help='Cumulative plot use buffer size from all peaks instead of peak-only.')
    parser.add_argument('--skip-buffer-dist', action='store_true', help='Skip the buffer distribution plot.')
    parser.add_argument('--skip-buffer-over-time', action='store_true', help='Skip the buffer-over-time plot.')
    parser.add_argument('--total-only', action='store_true', help='Plot only the plot for the sum of buffering between all fifos.')
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
    outdir = "output_{}".format(args.inpath.split("/")[-1])
    if not os.path.exists(outdir):
        os.makedirs(outdir)
    cache_file_name = "{}/data.pkl".format(outdir)
    data = load_data(args.inpath, cache_file_name)
    if not args.skip_buffer_dist:
        plot_buffer_distribution(data, outdir, args)
    if not args.skip_buffer_over_time:
        plot_buffer_over_time(data, outdir, args)
    return

if __name__ == "__main__":
    main()
