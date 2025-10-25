#!/usr/bin/env python3

# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# This script uses the control_system monitor shell command to get all
# control system variable values and visualize these variables on a live
# graph. The script additionally can log the output to a simple json file,
# which can be used for further scripting, and can read and visualized by
# this script.
#
# To visualize all control system variables, the following command can be used.
#
#     ./control_system.py --serial-port /dev/pts/2 --baudrate 115200
#
# To log the control system variables, specify an output path for the logfile:
#
#     ./control_system.py --serial-port /dev/pts/2 --baudrate 115200 \
#     --logfile log.json
#
# To read and visualize an existing log file, specify only the existing
# logfile
#
#     ./control_system.py --logfile log.json
#
# To only visualize and log a subset of the control systems variables present
# on the device, specify them by name with the --variable option:
#
#     ./control_system.py --serial-port /dev/pts/2 --logfile loj.json \
#     --variable foo_var --variable bar_var
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

    def _parse_q31_(self, val: int) -> float:
        return self._limit_(float(val) / 2147483648)

    def _parse_usec_(self, val: int) -> float:
        return self._limit_(float(val) / 1000000)

    def parseline(self, line: str) -> dict:
        content = json.loads(line)
        return {
            "name": content[0],
            "timestamp": self._parse_usec_(content[1]),
            "value": self._parse_q31_(content[2])
        }

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
        # sorted by control system variable name.
        self.variables = {}
        self._samplelimit = samplelimit
        self._whitelist = whitelist

    def __del__(self):
        if self._file:
            # Trim trailing comma
            self._file.seek(self._file.tell() - 2, 0)
            # Termiante json file
            self._file.write('\n\t]\n}\n')

    def log(self, sample: dict):
        if self._whitelist and sample['name'] not in self._whitelist:
            return

        if self._file:
            self._file.write(f'\t\t{json.dumps(sample)},\n')

        if sample['name'] not in self.variables:
            self.variables[sample['name']] = {
                'timestamps': [],
                'values': [],
            }

        self.variables[sample['name']]['timestamps'].append(sample['timestamp'])
        self.variables[sample['name']]['values'].append(sample['value'])

        if self._samplelimit:
            for name in self.variables:
                if len(self.variables[name]['timestamps']) > self._samplelimit:
                    self.variables[name]['timestamps'] = self.variables[name]['timestamps'][1:]
                    self.variables[name]['values'] = self.variables[name]['values'][1:]

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
        self._colors = ['red', 'blue', 'orange', 'green', 'purple', 'black']

    def _remove_plots_(self):
        for graph in self._graphs:
            graph.remove()
        self._graphs = []

    def _add_plots_(self, logger):
        plt.ylim(-1.1, 1.1)
        plt.xlabel("seconds")
        titles = []
        xlim_min = 0xFFFFFFFF
        xlim_max = 0
        for i, name in enumerate(logger.variables):
            timestamps = logger.variables[name]['timestamps']
            xlim_min = min(xlim_min, min(timestamps))
            xlim_max = max(xlim_max, max(timestamps))
            values = logger.variables[name]['values']
            color = self._colors[i]
            titles.append(f'{color}: {name}')
            self._graphs.append(plt.plot(timestamps, values, color = color)[0])
        plt.title(', '.join(titles))
        plt.xlim(xlim_min, xlim_max)

    def _prepare_view_(self, logger):
        self._remove_plots_()
        self._add_plots_(logger)

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
    parser.add_argument("--variable", action='append',
                        help="Specifies subset of control system variables to include")
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
        logger = Logger(args.logfile, args.samplelimit, args.variable)
        try:
            while True:
                lines = backend.readlines(0.05)
                for line in lines:
                    try:
                        sample = parser.parseline(line)
                    except:
                        continue
                    logger.log(sample)
                if len(logger.variables):
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
