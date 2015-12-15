#!/usr/bin/env python

"""
Extract iperf data from json blob and format for gnuplot.
"""

import json
import os
import sys

import os.path
from optparse import OptionParser

import pprint
# for debugging, so output to stderr to keep verbose
# output out of any redirected stdout.
pp = pprint.PrettyPrinter(indent=4, stream=sys.stderr)

def generate_output(iperf, options):
    for i in iperf.get('intervals'):
        for ii in i.get('streams'):
            if options.verbose: pp.pprint(ii)
            row = '{0} {1} {2} {3} {4}\n'.format(
                        round(float(ii.get('start')), 4),
                        ii.get('bytes'),
                        # to Gbits/sec
                        round(float(ii.get('bits_per_second')) / (1000*1000*1000), 3),
                        ii.get('retransmits'),
                        round(float(ii.get('snd_cwnd')) / (1000*1000), 2)
                    )
            yield row

def main():
    usage = '%prog [ -f FILE | -o OUT | -v ]'
    parser = OptionParser(usage=usage)
    parser.add_option('-f', '--file', metavar='FILE',
        type='string', dest='filename', 
        help='Input filename.')
    parser.add_option('-o', '--output', metavar='OUT',
            type='string', dest='output', 
            help='Optional file to append output to.')
    parser.add_option('-v', '--verbose',
        dest='verbose', action='store_true', default=False,
        help='Verbose debug output to stderr.')
    options, args = parser.parse_args()
    
    if not options.filename:
        parser.error('Filename is required.')
    
    file_path = os.path.normpath(options.filename)
    
    if not os.path.exists(file_path):
        parser.error('{f} does not exist'.format(f=file_path))

    with open(file_path,'r') as fh:
        data = fh.read()
    
    try:
        iperf = json.loads(data)
    except Exception as e:
        parser.error('Could not parse JSON from file (ex): {0}'.format(str(e)))

    if options.output:
        absp = os.path.abspath(options.output)
        d,f = os.path.split(absp)
        if not os.path.exists(d):
            parser.error('Output file directory path {0} does not exist'.format(d))
        fh = open(absp, 'a')
    else:
        fh = sys.stdout

    for i in generate_output(iperf, options):
        fh.write(i)


if __name__ == '__main__':
    main()