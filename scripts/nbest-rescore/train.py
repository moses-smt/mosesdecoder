#!/usr/bin/env python
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

import argparse
import os
import subprocess
import sys

# Feature field in N-best format
FEAT_FIELD = 2

# Location of mert, kbmira, etc. in relation to this script
BIN_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(__file__))), 'bin')

def main():

    # Args
    parser = argparse.ArgumentParser(description='Learn N-best rescoring weights')
    parser.add_argument('--nbest', metavar='nbest', \
            help='Dev set N-best list augmented with new features', required=True)
    parser.add_argument('--ref', metavar='ref', \
            help='Dev set reference translation', required=True)
    parser.add_argument('--working-dir', metavar='rescore-work', \
            help='Optimizer working directory', required=True)
    parser.add_argument('--bin-dir', metavar='DIR', \
            help='Moses bin dir, containing kbmira, evaluator, etc.', default=BIN_DIR)
    # Since we're starting with uniform weights and only running kbmira once,
    # run a gratuitous number of iterations.  (mert-moses.pl default is 60
    # iterations for each Moses run)
    parser.add_argument('--iterations', metavar='N', type=int, \
            help='Number of K-best MIRA iterations to run (default: 300)', default=300)
    args = parser.parse_args()

    # Find executables
    extractor = os.path.join(args.bin_dir, 'extractor')
    kbmira = os.path.join(args.bin_dir, 'kbmira')
    for exe in (extractor, kbmira):
        if not os.path.exists(exe):
            sys.stderr.write('Error: cannot find executable "{}" in "{}", please specify --bin-dir\n'.format(exe, args.bin_dir))
            sys.exit(1)

    # rescore-work dir
    if not os.path.exists(args.working_dir):
        os.mkdir(args.working_dir)

    # Feature names and numbers of weights from N-best list
    # Assume all features are dense (present for each entry)
    init_weights = []
    fields = [f.strip() for f in open(args.nbest).readline().split('|||')]
    feats = fields[FEAT_FIELD].split()
    for i in range(len(feats)):
        if feats[i].endswith('='):
            n_weights = 0
            j = i + 1
            while j < len(feats):
                if feats[j].endswith('='):
                    break
                n_weights += 1
                j += 1
            # Start all weights at 0
            init_weights.append([feats[i], [0] * n_weights])

    # Extract score and feature data from N-best list
    extractor_cmd = [extractor, \
            '--sctype', 'BLEU', '--scconfig', 'case:true', \
            '--scfile', os.path.join(args.working_dir, 'scores.dat'), \
            '--ffile', os.path.join(args.working_dir, 'features.dat'), \
            '-r', args.ref, \
            '-n', args.nbest]
    subprocess.call(extractor_cmd)

    # Write dense feature list
    with open(os.path.join(args.working_dir, 'init.dense'), 'w') as out:
        for (feat, weights) in init_weights:
            for w in weights:
                out.write('{} {}\n'.format(feat, w))

    # Run K-best MIRA optimizer
    kbmira_cmd = [kbmira, \
            '--dense-init', os.path.join(args.working_dir, 'init.dense'), \
            '--ffile', os.path.join(args.working_dir, 'features.dat'), \
            '--scfile', os.path.join(args.working_dir, 'scores.dat'), \
            '-o', os.path.join(args.working_dir, 'mert.out'), \
            '--iters', str(args.iterations)]
    subprocess.call(kbmira_cmd)

    # Read optimized weights, sum for normalization
    opt_weights = []
    total = 0
    with open(os.path.join(args.working_dir, 'mert.out')) as inp:
        # Same structure as original weight list
        for (feat, weights) in init_weights:
            opt_weights.append([feat, []])
            for _ in weights:
                w = float(inp.readline().split()[1])
                opt_weights[-1][1].append(w)
                # Sum for normalization
                total += abs(w)

    # Normalize weights
    for (_, weights) in opt_weights:
        for i in range(len(weights)):
            weights[i] /= total

    # Generate rescore.ini
    with open(os.path.join(args.working_dir, 'rescore.ini'), 'w') as out:
        out.write('# For use with Moses N-best rescorer "scripts/nbest-rescore/rescore.py"\n')
        out.write('\n')
        out.write('[weight]\n')
        for (feat, weights) in opt_weights:
            out.write('{} {}\n'.format(feat, ' '.join(str(w) for w in weights)))

if __name__ == '__main__':
    main()
