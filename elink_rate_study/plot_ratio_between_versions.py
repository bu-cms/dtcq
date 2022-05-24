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
from plot_distribution import basename_to_module, module_to_layout, load_data_from_tree, sort_data_to_elinks, dtc_names, detector_sections, organize_data

mpl.rcParams['axes.prop_cycle'] = cycler("color", plt.get_cmap("tab20c").colors)

plt.style.use(hep.style.CMS)


def plot_ratio(data1, data2, distribution, args, outdir, labels=["all"], plot_tag="all", ylabel=None):
    # parse additional args
    stack = args.stack
    fig,ax = plt.subplots(1,1)
    print("Plotting: ratio of tag={} distribution={}, labels={}, stack={}".format(args.tag, distribution, labels, stack))
    # apply unsimulated effects
    for label in labels:
        if not len(data1[label]) == len(data2[label]):
            print("length not matched:")
            print("\tlabel=",label)
            print("\tlen(data1)=",len(data1[label]))
            print("\tlen(data2)=",len(data2[label]))
    x = [data2[label]/data1[label] for label in labels]
    if stack:
        ax.hist(x, bins=30, stacked=True, label=labels)
    else:
        ax.hist(x, bins=30, density=True, histtype="step", lw=3, alpha=0.8, label=labels)
    if stack:
        if ylabel == None:
            ylabel = "N(elinks)"
        ax.set_ylabel(ylabel)
    else:
        ax.set_ylabel("distribution")
    if "relative_std" in distribution:
        ax.set_xlabel("relative std")
    elif "occupancy" in distribution:
        ax.set_xlabel("occupancy ratio")
        ax.axvline(x=1., color="green", lw=5, ls="--")
        #ax.axvline(x=100, color="red",   lw=5, ls="--")
        # temporary
        ax.set_xlim(0.85,1.15)
        #ax.set_xlim(0.65,1.35)
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
    if not os.path.exists(outdir):
        os.makedirs(outdir)
    if stack:
        plot_tag = plot_tag + "_stack"
    plt.text(0.3, 0.8, "mean={:3.3f}\nstd={:3.3f}".format(np.concatenate(x).mean(), np.concatenate(x).std()), horizontalalignment='right', verticalalignment='bottom', transform=ax.transAxes)
    plot_file_name = "{}/{}_{}.png".format(outdir, distribution, plot_tag)
    fig.savefig(plot_file_name)
    print("saved plots {}".format(plot_file_name))
    del ax,fig

def command_line():
    parser = argparse.ArgumentParser(prog="Plotter.")
    parser.add_argument("tag", type=str, help="tag for naming.")
    parser.add_argument("pkl1", type=str, help="first pkl file for comparison.")
    parser.add_argument("pkl2", type=str, help="second pkl file for comparison.")
    parser.add_argument("--area", type=str, default="all", help="area of the detector to plot. Can be all, bydtc, bysection, TBPX_L1 etc.")
    parser.add_argument("--bychip", action="store_true", help="each chip gets an entry, compared to each entry for an elink.")
    parser.add_argument("--distribution", type=str, default="occupancy", help="distribution to plot, can be \"size\" or \"occupancy\".")
    parser.add_argument("--individual", action="store_true", help="also plot the individual plots. effective when area=bydtc or area=bysection.")
    parser.add_argument("--stack", action="store_true", help="wether we stack the hists or not.")
    args = parser.parse_args()
    return args

def main():
    args = command_line()
    tag = args.tag
    outdir = "comparison/{}".format(tag)
    if not os.path.exists(outdir):
        os.makedirs(outdir)
    pkl_filenames = [args.pkl1, args.pkl2]
    print("ratio = {} / {}".format(args.pkl2, args.pkl1))
    datas = [None, None]
    for i in range(2):
        with open(pkl_filenames[i], "rb") as f:
            print("loading data from file:"+pkl_filenames[i]+"...")
            datas[i] = pkl.load(f)
            if "lumi" in pkl_filenames[i]:
                datas[i] = add_lumi(datas[i])
            datas[i] = organize_data(datas[i],args.distribution)
    distribution = args.distribution

    if not "elinks" in pkl_filenames[0]:
        ylabel = "N(chips)"
    else:
        ylabel = "N(elinks)"

    # make the plots
    if args.area=="all":
        plot_ratio(*datas, distribution, args, outdir, labels=["all"], plot_tag="all", ylabel=ylabel)
    elif args.area=="bydtc":
        plot_ratio(*datas, distribution, args, outdir, labels=dtc_names, plot_tag="dtcs", ylabel=ylabel)
        if args.individual:
            for dtc_name in dtc_names:
                plot_ratio(*datas, distribution, args, outdir, labels=[dtc_name], plot_tag=dtc_name, ylabel=ylabel)
    elif args.area=="bysection":
        plot_ratio(*datas, distribution, args, outdir, labels=list(detector_sections.keys()), plot_tag="sections", ylabel=ylabel)
        if args.individual:
            for section_name in detector_sections.keys():
                plot_ratio(*datas, distribution, args, outdir, labels=[section_name], plot_tag=section_name.replace(" ", "_"), ylabel=ylabel)
    else:
        section_name = args.area.replace("_"," ")
        plot_ratio(*datas, distribution, args, outdir, labels=[section_name], plot_tag=section_name.replace(" ", "_"), ylabel=ylabel)

    return 0

if __name__ == "__main__":
    main()
