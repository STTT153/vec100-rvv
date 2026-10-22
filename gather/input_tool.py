#!/usr/bin/env python3
"""Standard-library reader/writer for gather-input v1. No pointer addresses on disk."""
import argparse
import pathlib
import struct

FORMATS = dict(u8='B', i8='b', u16='H', i16='h', u32='I', i32='i',
               u64='Q', i64='q', f32='f', f64='d')

def read(directory):
    directory = pathlib.Path(directory)
    lines = (directory / 'manifest.txt').read_text().splitlines()
    if not lines or lines[0] != 'gather-input 1':
        raise ValueError('unsupported manifest')
    result = dict(params={}, arrays={}, refs={})
    for line in lines[1:]:
        if not line or line.startswith('#'):
            continue
        tag, name, *rest = line.split()
        if tag in ('kernel', 'entry'):
            result[tag] = name
        elif tag == 'param':
            result['params'][name] = int(rest[0])
        elif tag == 'array':
            typ, count = rest
            data = (directory / (name + '.bin')).read_bytes()
            if len(data) != int(count) * struct.calcsize('<' + FORMATS[typ]):
                raise ValueError('array length mismatch: ' + name)
            # Retain raw bytes so NaN payloads and signed zeros survive roundtrips.
            result['arrays'][name] = (typ, data)
        elif tag == 'ref':
            slot, target, offset = rest
            refs = result['refs'].setdefault(name, [])
            if int(slot) != len(refs):
                raise ValueError('nonconsecutive reference slots')
            refs.append((target, int(offset)))
        else:
            raise ValueError('unknown record: ' + tag)
    return result

def values(case, name):
    typ, data = case['arrays'][name]
    return [x[0] for x in struct.iter_unpack('<' + FORMATS[typ], data)]

def set_values(case, name, values):
    typ, _ = case['arrays'][name]
    case['arrays'][name] = typ, b''.join(struct.pack('<' + FORMATS[typ], v) for v in values)

def write(case, directory):
    directory = pathlib.Path(directory)
    directory.mkdir(parents=True, exist_ok=False)
    lines = ['gather-input 1', 'kernel ' + case['kernel'], 'entry ' + case['entry']]
    for name, value in sorted(case['params'].items()):
        lines.append(f'param {name} {value}')
    for name, (typ, data) in sorted(case['arrays'].items()):
        count, remainder = divmod(len(data), struct.calcsize('<' + FORMATS[typ]))
        if remainder:
            raise ValueError('misaligned array payload')
        lines.append(f'array {name} {typ} {count}')
        (directory / (name + '.bin')).write_bytes(data)
    for name, refs in sorted(case['refs'].items()):
        for slot, (target, offset) in enumerate(refs):
            lines.append(f'ref {name} {slot} {target} {offset}')
    (directory / 'manifest.txt').write_text('\n'.join(lines) + '\n')

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input')
    parser.add_argument('--copy-to')
    args = parser.parse_args()
    case = read(args.input)
    print(case['kernel'], case['entry'], case['params'])
    for name, (typ, data) in case['arrays'].items():
        print(name, typ, len(data), 'bytes')
    if args.copy_to:
        write(case, args.copy_to)

if __name__ == '__main__':
    main()
