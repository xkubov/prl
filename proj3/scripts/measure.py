#!/usr/bin/env python3

from argparse import ArgumentParser
from collections import defaultdict
import datetime
import itertools
import json
import math
import random
import signal
import subprocess
import sys
from typing import Optional

FAIL = '\033[91m\033[1m'
GREEN = '\033[32m\033[1m'
ENDC = '\033[0m'
BOLD = '\033[1m'

data = defaultdict(list)

def process_item(inputSize: int) -> Optional[int]:
    altitudes = [random.randint(1, 1024) for _ in range(inputSize)]
    print(altitudes)

    args = ['./test.sh', '']

    print(f'[Input size] {inputSize}')
    # permutations = itertools.permutations(altitudes)

    args[1] = ','.join(map(str, altitudes))
    print(f'[Input string] {args[1]}')
    angles = [-math.inf] + [math.atan((x - altitudes[0]) / float(i))
            for i, x in enumerate(altitudes[1:], 1)]
    print(angles)

    max_prescan = [-math.inf] + list(itertools.accumulate(angles, max))
    max_prescan.pop()
    visibilities = ["v" if (angle > max_prev_angle) else "u"
                    for (angle, max_prev_angle) in zip(angles, max_prescan)]
    visibilities[0] = "_"

    ref_output = ",".join(visibilities)
    result = subprocess.run(args,
                            universal_newlines=True,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    try:
        time = int(result.stderr)
    except:
        time = -1
    output = result.stdout.rstrip('\n')
    if output != ref_output:
        print(f"{FAIL}FAIL: {ENDC}{FAIL}{output}{ENDC} != {GREEN}{ref_output}{ENDC}")

    # Return time record.
    return time

def handler(signum, frame):
    global data
    # Serialize the results to json
    with open(f'handler_experiment_{datetime.datetime.now():%Y-%m-%d_%H-%M}.json', 'w') as f:
        f.write(json.dumps(data))

    sys.exit(1)

def main():
    signal.signal(signal.SIGINT, handler)
    global data

    parser = ArgumentParser()
    parser.add_argument("-n", "--max_numbers", type=int, default=30)
    parser.add_argument("-i", "--iterations", type=int, default=32)

    argv = parser.parse_args()

    # Run and track program.
    print('Running...', end='')
    for num in reversed(range(1, argv.max_numbers)):
        for _ in range(argv.iterations):
            try:
                t = process_item(num)
                if t is not None:
                    data[num].append(t)
                else:
                    print('Command failed... ignoring time!')
            except subprocess.TimeoutExpired as e:
                print('Command timeouted, trying {} processors...'.format(num - 1))
                print(e.stderr)
    print('\rDone      ')

    # Serialize the results to json
    with open(f'experiment_{datetime.datetime.now():%Y-%m-%d_%H-%M}.json', 'w') as f:
        f.write(json.dumps(data))

if __name__ == '__main__':
    main()
