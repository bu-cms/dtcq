#!/bin/env python

import argparse
import re,os,sys
import numpy as np
from matplotlib import pyplot as plt
from scipy.special import erf
from scipy.stats import norm, linregress, chisquare
from scipy.optimize import curve_fit

FPGA_FREQUENCY = 400000 #kHz

def commandline():
    parser = argparse.ArgumentParser(prog='Plotter.')
    parser.add_argument('inpath', type=str, help='Input folder to use.')
    args = parser.parse_args()
    return args

def fmax_norm(x, n):
    return norm().pdf(x)*(0.5*(1+erf(x)))**n
def fmax_norm_1(x, n):
    return (n/np.sqrt(0.5*np.pi))*np.exp(-x*x - (n)*np.exp(-x*x)/np.sqrt(np.pi)/x)
def fmax_exp(x, n):
    return np.exp(-x+np.log(n)) * np.exp(-np.exp(-x+np.log(n)))
def f_exp(x, a, n, A):
    return A*a*fmax_exp(a*x, n)
def f_norm(x, a, n, A):
    return A*a*fmax_norm(a*x,n)

def extract_period(path):
    pattern = re.compile(".*MaxOnly(\d+).*")
    match = pattern.match(path)
    if match:
        return int(match.groups()[0])
    else:
        raise ValueError("unable to extract period from input path")

def fit_eva(data, period, fifo_name, ostream, outdir):
    # bootstrapping
    multipliers = [1,2,4,8,16,32,64]
    centers = []
    resampled_max = {}
    fig, ax = plt.subplots(1,1)
    for multiplier in multipliers:
        n_resampling = 100000
        resampled_raw = np.random.choice(data, (n_resampling, multiplier))
        resampled_max[multiplier] = resampled_raw.max(axis=1)
        centers.append(np.median(resampled_max[multiplier]))
        ax.hist(resampled_max[multiplier]*64/1000, density=True, bins=50, label="{} ms".format(0.13*multiplier))
    ax.set_xlabel(r"max occpancy within $\tau$ (Kb)")
    ax.set_ylabel("density")
    ax.legend()
    fig.savefig(f"{outdir}/histograms.png")

    # linear fit
    tau = period / FPGA_FREQUENCY
    lin_x = np.log10(tau*np.array(multipliers))
    lin_y = [c*64/1000 for c in centers]
    def lin(x, a, b):
        return a*x + b
    ostream.write(f"========={fifo_name}==========\n")
    print(f"========={fifo_name}==========\n")
    popt, popv = curve_fit(lin, lin_x, lin_y)
    ostream.write("slope={:.3f}, intercept={:.3f}\n".format(*popt))
    print("slope={:.3f}, intercept={:.3f}\n".format(*popt))
    # save fit linear fit plot
    slope, intercept = popt
    ax.clear()
    ax.plot(lin_x, lin_y, '+', label='data')
    ax.plot(lin_x, slope*lin_x + intercept, label='fit {:3.2f}x+{:3.2f}'.format(slope, intercept))
    ax.set_xlabel(r"$\log_{10}(\tau/ms)$")
    ax.set_ylabel("memory usage (kb)")
    ax.legend()
    fig.savefig(f"{outdir}/linfit.png")
    return popt, popv

def print_mean_intervals(slope, intercept, fifo_name, ostream):
    mem_values = [18*i for i in range(1,5)]
    ostream.write("| {:<15} | {:<15} |".format("threshold", "mean interval"))
    print("| {:<15} | {:<15} |".format("threshold", "mean interval"))
    for mem_value in mem_values:
        half_life = np.power(10, (mem_value - intercept)/slope - 3) # in s
        mean_interval = half_life/np.log(2) # in min
        ostream.write("| {:<15} | {:<15} |".format(mem_value, mean_interval))
        print("| {:<15} | {:<15} |".format(mem_value, mean_interval))


def main():
    args = commandline()
    while args.inpath[-1] == "/":
        args.inpath = args.inpath[:-1]
    outdir = "EVA_result/{}".format(args.inpath.split("/")[-1])
    if not os.path.exists(outdir):
        os.makedirs(outdir)
    with open(f"{outdir}/log.txt", "w") as ostream:
        for fifo_name in ["input_fifo", "output_fifo_data"]:
            fname = f"{args.inpath}/period_max_{fifo_name}.bin"
            data = np.fromfile(fname, dtype=np.uint16)
            period = extract_period(args.inpath)
            popt, popv = fit_eva(data, period, fifo_name, ostream, outdir)
            slope, intercept = popt
            var_slope = popv[0][0]
            var_intercept = popv[1][1]
            print_mean_intervals(slope, intercept, fifo_name, ostream)
    return

if __name__ == "__main__":
    main()
