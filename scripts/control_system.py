#!/usr/bin/env python3

# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# This script uses the control_system monitor shell command to get the
# control system variables of all control systems on the device, and
# visualize these variables for all, or a subset of, the control
# systems on live graphs. The script additionally can log the output
# to a simple json file, which can be used for further scripting,
# and can read and visualized by this script.
#
# To visualize all control systems, the following command can be used.
#
#     ./control_system.py --serial-port /dev/pts/2 --baudrate 115200
#
# To log the control systems, specify an output path for the logfile:
#
#     ./control_system.py --serial-port /dev/pts/2 --baudrate 115200 \
#     --logfile log.json
#
# To read and visualize an existing log file, specify only the existing
# logfile
#
#     ./control_system.py --logfile log.json
#
# To only visualize and log a subset of the control systems present on
# the device, specify them by name with the --control-system option:
#
#     ./control_system.py --serial-port /dev/pts/2 --logfile loj.json \
#     --control-system foo --control-system bar
#
# The samplerate and samplelimit can be adjusted with the --samplerate
# and --samplelimit options. Take care not to set too high a samplerate
# as this may overwhelm the device and/or the UART.

import os
import serial
import argparse
import matplotlib.pyplot as plt
import json
import datetime
import time

class Backend():
    def __init__(self, serial_port: str, baudrate: int):
        self.sp = serial.Serial(serial_port, baudrate, timeout=0.2)
        self.writeline('')
        self.flush()

    def writeline(self, line: str):
        self.sp.write(f'{line}\n'.encode('ascii'))

    def flush(self):
        self.sp.read(1024)

    def readline(self) -> str:
        endline = bytes([13, 10])
        received = self.sp.read_until(endline)
        if len(received) == 0:
            return ''
        if received.endswith(endline):
            return received[:-2].decode('ascii')
        received += self.sp.read_until(endline)
        if received.endswith(endline):
            return received[:-2].decode('ascii')
        return ''

    def readlines(self, timeout: float) -> list[str]:
        deadline = time.monotonic() + timeout
        lines = []
        while time.monotonic() < deadline:
            line = self.readline()
            if line == '':
                return lines
            lines.append(line)
        return lines

    def eox(self):
        self.sp.write(bytes([3]))

class Parser():
    def _limit_(self, val: float) -> float:
        if val < 0.0001 and val > 0:
            return 0.0001
        elif val > -0.0001 and val < 0:
            return -0.0001
        return val

    def parseline(self, line: str) -> dict:
        content = json.loads(line)
        # Limit "small" values to avoid python using scientific
        # notation when printing "small" floating point values.
        content['sp'] = self._limit_(content['sp'])
        content['pv'] = self._limit_(content['pv'])
        content['sa'] = self._limit_(content['sa'])
        return content

class Logger():
    def __init__(self, logfile: str | None, samplelimit: int=None, whitelist: list[str]=None):
        if logfile:
            assert not os.path.exists(logfile), "logfile already exists"
            self._file = open(logfile, 'w')

            # We will be partially building a json file so we format it
            # manually. This avoids dumping hours of samples from RAM
            # to the log file when the script terminates, and if
            # something goes wrong, the samples will be present in the
            # partially written file to be manually recovered.
            self._file.write('{\n')
            self._file.write('\t"version": "0.0.0",\n')
            self._file.write(f'\t"created": "{datetime.datetime.now().isoformat()}",\n')
            self._file.write(f'\t"samples": [\n')
        else:
            self._file = None

        # We preprocess each sample when it is added to the logger,
        # splitting it into arrays which are directly usable by pyplot,
        # sorted by control system name.
        self.control_systems = {}
        self._samplelimit = samplelimit
        self._whitelist = whitelist

    def __del__(self):
        if self._file:
            # Trim trailing comma
            self._file.seek(self._file.tell() - 2, 0)
            # Termiante json file
            self._file.write('\n\t]\n}\n')

    def log(self, sample: dict):
        if self._whitelist and sample['nm'] not in self._whitelist:
            return

        if self._file:
            self._file.write(f'\t\t{json.dumps(sample)},\n')

        if sample['nm'] not in self.control_systems:
            self.control_systems[sample['nm']] = {
                'ts': [],
                'sp': [],
                'pv': [],
                'sa': [],
            }

        self.control_systems[sample['nm']]['ts'].append(sample['ts'])
        self.control_systems[sample['nm']]['sp'].append(sample['sp'])
        self.control_systems[sample['nm']]['pv'].append(sample['pv'])
        self.control_systems[sample['nm']]['sa'].append(sample['sa'])

        if self._samplelimit:
            for nm in self.control_systems:
                if len(self.control_systems[nm]['ts']) > self._samplelimit:
                    self.control_systems[nm]['ts'] = self.control_systems[nm]['ts'][1:]
                    self.control_systems[nm]['sp'] = self.control_systems[nm]['sp'][1:]
                    self.control_systems[nm]['pv'] = self.control_systems[nm]['pv'][1:]
                    self.control_systems[nm]['sa'] = self.control_systems[nm]['sa'][1:]

class Log():
    def __init__(self, logfile: str):
        assert os.path.exists(logfile), "logfile does not exists"
        with open(logfile, 'r') as file:
            content = json.load(file)
        self.version = content['version']
        self.created = content['created']
        self.samples = content['samples']

class Viewer():
    def __init__(self):
        self._graphs = []
        self._colors = {'sp': 'g', 'pv': 'b', 'sa': 'r'}

    def _remove_plots_(self):
        for graph in self._graphs:
            graph.remove()
        self._graphs = []

    def _add_plot_(self, nm: str, cs: dict):
        plt.title(nm + ' (green = sp, blue = pv, red = sa)')
        plt.xlabel("")
        plt.ylim(-1.1, 1.1)
        if cs['ts'][0] < cs['ts'][-1]:
            plt.xlim(cs['ts'][0], cs['ts'][-1])
        self._graphs.append(plt.plot(cs['ts'], cs['sp'], color = self._colors['sp'])[0])
        self._graphs.append(plt.plot(cs['ts'], cs['pv'], color = self._colors['pv'])[0])
        self._graphs.append(plt.plot(cs['ts'], cs['sa'], color = self._colors['sa'])[0])

    def _prepare_view_(self, logger):
        self._remove_plots_()
        for i, nm in enumerate(logger.control_systems):
            plt.subplot(len(logger.control_systems), 1, i + 1)
            self._add_plot_(nm, logger.control_systems[nm])
        plt.xlabel("seconds")

    def run(self, logger, interval: float):
        plt.ion()
        self._prepare_view_(logger)
        plt.pause(interval)

    def show(self, logger):
        plt.ioff()
        self._prepare_view_(logger)
        plt.show()

    def is_open(self) -> bool:
        return True if plt.get_fignums() else False

def parse_args():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False
    )
    parser.add_argument("--serial-port",
                        help="serial port to read from")
    parser.add_argument("--baudrate", type=int, default=115200,
                        help="baudrate of serial port")
    parser.add_argument("--samplerate", type=int, default=25,
                        help="samplerate in samples per second")
    parser.add_argument("--logfile",
                        help="path to json formatted log file")
    parser.add_argument("--samplelimit", type=int, default=500,
                        help="maximum number of samples displayed in viewer")
    parser.add_argument("--control-system", action='append',
                        help="Specifies subset of control systems to include")
    return parser.parse_args()

def main():
    args = parse_args()

    if not args.serial_port and not args.logfile:
        raise Exception("Either --serial-port or --logfile must be specified")

    if not args.serial_port and not os.path.exists(args.logfile):
        raise Exception("logfile does not exist")

    if args.serial_port:
        backend = Backend(args.serial_port, args.baudrate)
        interval_us = int((1 / args.samplerate) * 1000000)
        backend.writeline(f'control_system monitor {interval_us}')
        backend.flush()
        parser = Parser()
        viewer = Viewer()
        logger = Logger(args.logfile, args.samplelimit, args.control_system)
        try:
            while True:
                lines = backend.readlines(0.05)
                for line in lines:
                    try:
                        sample = parser.parseline(line)
                    except:
                        continue
                    logger.log(sample)
                if len(logger.control_systems):
                    viewer.run(logger, 0.05)
                    if not viewer.is_open():
                        break
        except Exception as e:
            print(e)
        finally:
            backend.eox()
    else:
        log = Log(args.logfile)
        logger = Logger(None)
        viewer = Viewer()
        for sample in log.samples:
            logger.log(sample)
        viewer.show(logger)

if __name__=="__main__":
    main()
