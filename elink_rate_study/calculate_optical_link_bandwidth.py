import numpy as np
from collections import defaultdict
import re

config_fn = "../config/v5.config"

bw_sum = defaultdict(float)
pattern = re.compile("dtc(\d+).*")

with open(config_fn) as fin:
    for line in fin.readlines():
        basename, _, _, rate = line.strip().split()
        match = pattern.match(basename)
        dtc = int(match.groups()[0])
        bw_sum[dtc] += float(rate)

for idx in range(1,8):
    dtc = 10+idx
    print((dtc, bw_sum[dtc]*64*0.75e-3, bw_sum[dtc]*64*0.75e-3/(12*25)))
