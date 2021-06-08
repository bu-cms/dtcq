#!/bin/env python

import numpy as np
from matplotlib import pyplot as plt

fifo_sizes = [[] for i in range(60)]
input_fn = "../output/output_fifo_data_sizes.txt"
with open(input_fn) as f:
    for iline,line in enumerate(f.readlines()):
        numbers = line.replace("\n","").split(",")
        #print(numbers)
        for i in range(len(fifo_sizes)):
            #print(int(numbers[i]))
            fifo_sizes[i].append(int(numbers[i]))
        #print(fifo_sizes)
fifo_sizes = [np.array(i) for i in fifo_sizes]
#print(fifo_sizes)
#max_buffer = [max(i) for i in fifo_sizes]
#print(max_buffer)

#indexies = range(len(max_buffer))
#indexies.sort(key=max_buffer.__get_item__, reverse=True)
#print(indexies)
#exit()

lines_on_plot = 0
max_lines_on_plot = 5
fig,ax = plt.subplots(1,1)
for i,y in enumerate(fifo_sizes):
    x = np.arange(len(y))
    ax.plot(x,y,label="FIFO {}".format(i))
    lines_on_plot += 1
    if (lines_on_plot==5 or i==len(fifo_sizes)-1):
        plot_file_name = "buffer_size_fifos{}-{}.png".format(i+1-lines_on_plot, i)
        lines_on_plot = 0
        ax.set_xlabel("clock tick")
        ax.set_ylabel("buffer size")
        ax.legend()
        fig.savefig(plot_file_name)
        ax.clear()
