#!/usr/bin/env python3
"""Run after building native originals or RVV binaries. Optional --runner CMD... ."""
import argparse
import csv
import pathlib
import subprocess
import tempfile
import sys
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
import input_tool
ROOT = pathlib.Path(__file__).resolve().parents[2]
IDS = '06 16 25 30 31 32 33 41 44 47 55 61 69 95 98'.split()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runner', nargs=argparse.REMAINDER, default=[])
    args = parser.parse_args()
    count = 0
    with tempfile.TemporaryDirectory(prefix='gather-test-') as temp:
        temp = pathlib.Path(temp)
        def run(binary, *flags, good=True):
            proc = subprocess.run(args.runner + [str(binary)] + list(map(str, flags)), text=True, capture_output=True)
            if (proc.returncode == 0) != good:
                raise AssertionError(f'{binary} {flags}: {proc.stdout}\n{proc.stderr}')
            return proc
        for kid in IDS:
            binary = next((ROOT / 'kernels').glob(kid + '_*')) / 'build/benchmark'
            for entry in run(binary, '--list-entries').stdout.splitlines():
                path = temp / (kid + '-' + entry)
                run(binary, '--entry', entry, '--generate', '--export-input', path, '--samples', 0)
                copy = path.with_name(path.name + '-copy')
                data = input_tool.read(path)
                input_tool.write(data, copy)
                export = path.with_name(path.name + '-export')
                run(binary, '--input', copy, '--export-input', export, '--samples', 0)
                assert input_tool.read(export) == data
                out1, out2 = path / 'one.csv', path / 'two.csv'
                run(binary, '--input', path, '--samples', 3, '--verify', '--output', out1)
                buffer = next(iter(data['arrays']))
                run(binary, '--input', copy, '--samples', 3, '--verify', '--output', out2,
                    '--memory-policy', 'warm', '--warm-buffer', buffer)
                rows1 = list(csv.DictReader(out1.open()))
                rows2 = list(csv.DictReader(out2.open()))
                assert len(rows1) == len(rows2) == 3
                assert rows1[0]['input_fnv1a64'] == rows2[0]['input_fnv1a64']
                assert all(r['verification'] == 'passed' for r in rows1 + rows2)
                run(binary, '--input', path, '--memory-policy', 'tcm', good=False)
                run(binary, '--input', path, '--generate', good=False)
                run(binary, '--input', path, '--memory-policy', 'warm', '--warm-buffer', 'absent', good=False)
                array = next(path.glob('*.bin'))
                array.write_bytes(array.read_bytes() + b'\x00')
                run(binary, '--input', path, good=False)
                print(kid, entry, 'passed', flush=True)
                count += 1
        # Invalid index and early exit on PCRE2, duplicate destination fallback in 95.
        binary = next((ROOT/'kernels').glob('95_*'))/'build/benchmark'
        valid = input_tool.read(temp/'95-recount-copy')
        input_tool.set_values(valid,'scan',[65535]*valid['params']['last'])
        bad = temp/'bad-index'; input_tool.write(valid,bad)
        run(binary,'--input',bad,good=False)
        valid = input_tool.read(temp/'95-recount-copy')
        input_tool.set_values(valid,'scan',[0]*valid['params']['last'])
        dup = temp/'duplicates';input_tool.write(valid,dup)
        run(binary,'--input',dup,'--samples',3,'--verify')
        binary = next((ROOT/'kernels').glob('69_*'))/'build/benchmark'
        valid = input_tool.read(temp/'69-match-copy')
        input_tool.set_values(valid,'bits',[255]*32)
        early = temp/'early';input_tool.write(valid,early)
        run(binary,'--input',early,'--samples',3,'--verify')
    print(f'{count} entries passed export/load, replay, warm, verification and rejection checks')

if __name__ == '__main__':
    main()
