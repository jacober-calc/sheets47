#!/usr/bin/env python3
"""Offline incremental build. Writes logs/artifacts here; never deletes files.

Usage: python3 -B tools/build.py host|device [--jobs N]
All commands and input fingerprints are recorded under build/<mode>/.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def run(command, log):
    temporary=log.parent/'tmp';temporary.mkdir(exist_ok=True)
    environment=os.environ.copy();environment['TMPDIR']=str(temporary)
    environment['SHEETS47_TEST_OUTPUT']=str(log.parent)
    result = subprocess.run(command, cwd=ROOT, env=environment, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    log.write_text(shlex.join(command) + '\n' + result.stdout)
    if result.returncode:
        print(result.stdout[-12000:], flush=True)
        raise RuntimeError(f'Command failed; see {log.relative_to(ROOT)}')
    return result.stdout


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('mode', choices=['host', 'host-sanitize', 'device'])
    p.add_argument('--jobs', type=int, default=2)
    args = p.parse_args()
    device = args.mode == 'device'
    out = ROOT / 'build' / args.mode
    out.mkdir(parents=True, exist_ok=True)
    tools = ROOT / '.local/tools/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi/bin'
    cxx = os.environ.get('ARM_CXX', str(tools / 'arm-none-eabi-g++')) if device else os.environ.get('CXX', 'g++')
    cc = os.environ.get('ARM_CC', str(tools / 'arm-none-eabi-gcc')) if device else 'gcc'
    if device:
        located=shutil.which(cxx)
        if located:tools=Path(located).resolve().parent
    includes = ['src', 'third_party/spreader/lib/code/inc', 'third_party/spreader/lib/code/src',
                'third_party/spreader/lib/code/generated', 'third_party/sys_string/lib/inc',
                'third_party/intrusive_shared_ptr/inc']
    flags = ['-std=c++20', '-Os', '-g', '-DNDEBUG', '-DSPR_SINGLE_THREADED=1', '-DSHEETS47_SMALL_GRID=1', '-ffunction-sections', '-fdata-sections',
             '-fno-threadsafe-statics', '-Wall', '-Wextra', '-Wno-unused-parameter']
    sanitizer=['-fsanitize=address,undefined','-fno-omit-frame-pointer'] if args.mode=='host-sanitize' else []
    flags+=sanitizer
    cpu = ['-mthumb', '-march=armv7e-m', '-mfloat-abi=hard', '-mfpu=fpv4-sp-d16'] if device else []
    if device:
        includes += ['third_party/dmcp5/dmcp', 'src/platform/dmcp5']
        flags += ['-std=gnu++20', '-DSHEETS47_DMCP5=1', '-DSYS_STRING_USE_GENERIC=1', '-fno-rtti', '-fstack-usage',
                  '-D__weak=__attribute__((weak))', '-D__packed=__attribute__((__packed__))']
    else:
        includes += ['third_party/dmcp5/dmcp']
        flags += ['-DSHEETS47_DMCP5_TEST=1', '-D__packed=__attribute__((__packed__))']
    flags += cpu + ['-I' + item for item in includes]
    engine = ROOT / 'third_party/spreader/lib/code'
    sources = sorted(engine.glob('src/*.cpp')) + sorted(engine.glob('src/functions/*.cpp')) + sorted(engine.glob('generated/*.cpp'))
    sources += [ROOT / 'third_party/sys_string/lib/src/unicode_mappings.cpp']
    sources += sorted((ROOT / 'src/engine').glob('*.cpp')) + sorted((ROOT / 'src/app').glob('*.cpp'))
    sources += sorted((ROOT / 'src/ui').glob('*.cpp')) + sorted((ROOT / 'src/storage').glob('*.cpp'))
    if device:
        sources += sorted((ROOT / 'src/platform/dmcp5').glob('*.cpp'))
        sources += [ROOT / 'third_party/dmcp5/dmcp/sys/pgm_syscalls.c', ROOT / 'third_party/dmcp5/dmcp/startup_pgm.s']
    else:
        sources += sorted((ROOT / 'src/platform/host').glob('*.cpp'))
        sources += [ROOT / 'src/platform/dmcp5' / name for name in ['files.cpp','input.cpp','setup.cpp']]
        sources += [ROOT / 'tests/host_tests.cpp', ROOT / 'tests/dmcp5_tests.cpp']
    # Header/source hashing avoids stale objects after header edits, with no
    # dependency-generator cleanup. Application is deliberately small for v0.1.
    headers = sorted((ROOT / 'src').rglob('*.h')) + sorted((ROOT / 'src').rglob('*.hpp'))
    headers += sorted((ROOT / 'third_party').rglob('*.h'))
    header_digest = hashlib.sha256(b''.join(x.read_bytes() for x in headers)).hexdigest()
    commands = []
    tasks = []
    objects = []
    for source in sources:
        relative = source.relative_to(ROOT)
        obj = out / (str(relative).replace('/', '_') + '.o')
        objects.append(obj)
        c_flags = flags if source.suffix == '.cpp' else cpu + ['-Os', '-g', '-ffunction-sections', '-fdata-sections'] + ['-I' + item for item in includes]
        if source.suffix != '.cpp':
            c_flags += ['-D__weak=__attribute__((weak))', '-D__packed=__attribute__((__packed__))']
        command = [cxx if source.suffix == '.cpp' else cc, *c_flags, '-c', str(relative), '-o', str(obj)]
        commands.append({'directory': str(ROOT), 'file': str(relative), 'arguments': command})
        fingerprint = hashlib.sha256(source.read_bytes() + header_digest.encode() + shlex.join(command).encode()).hexdigest()
        stamp = obj.with_suffix(obj.suffix + '.sha256')
        if obj.exists() and stamp.exists() and stamp.read_text() == fingerprint:
            continue
        tasks.append((source, obj, command, stamp, fingerprint))
    (out / 'compile_commands.json').write_text(json.dumps(commands, indent=2) + '\n')
    def compile_one(task):
        source, obj, command, stamp, fingerprint = task
        print('Compile', source.relative_to(ROOT), flush=True)
        run(command, obj.with_suffix('.log'))
        stamp.write_text(fingerprint)
    with ThreadPoolExecutor(max_workers=max(1, args.jobs)) as pool:
        list(pool.map(compile_one, tasks))
    target = out / ('sheets47.elf' if device else 'sheets47-tests')
    link = [cxx, *cpu, *sanitizer, *map(str, objects), '-Wl,--gc-sections', '-Wl,-Map=' + str(out / 'sheets47.map')]
    if device:
        link += ['-nostartfiles', '-Tsrc/platform/dmcp5/sheets47.ld', '-Wl,--wrap=_malloc_r', '-Wl,--wrap=_free_r', '-lm']
    else:
        link += ['-Wl,--wrap=malloc', '-Wl,--wrap=realloc']
    link += ['-o', str(target)]
    run(link, out / 'link.log')
    version = subprocess.check_output([cxx, '--version'], text=True).splitlines()[0]
    (out / 'build.json').write_text(json.dumps({'mode': args.mode, 'compiler': version, 'source_count': len(sources), 'headers_sha256': header_digest, 'link': link}, indent=2) + '\n')
    print('Linked', target.relative_to(ROOT), flush=True)
    if not device:
        result=run([str(target)], out / 'tests.log')
        print('\n'.join(line for line in result.splitlines() if not line.startswith(('Allocation sweep point','C allocation sweep point'))))
    else:
        objcopy = str(tools / 'arm-none-eabi-objcopy')
        run([objcopy, '--remove-section', '.qspi', '-O', 'binary', str(target), str(out / 'sheets47.bin')], out / 'objcopy.log')
        print(run(['python3', '-B', 'tools/package_pg5.py', str(out / 'sheets47.bin'), 'dist/sheets47-0.1.0/sheets47.pg5'], out / 'package.log'))
        sizes=run([str(tools / 'arm-none-eabi-size'), '-A', str(target)], out / 'size.log')
        sections={line.split()[0]:int(line.split()[1]) for line in sizes.splitlines() if line.startswith('.')}
        static_ram=sections.get('.data',0)+sections.get('.bss',0)
        if static_ram>8192 or sections.get('.qspi',0)!=0:raise RuntimeError('Static RAM or QSPI requirement violated')
        metrics={'sections':sections,'static_ram_bytes':static_ram,'flash_image_bytes':(out/'sheets47.bin').stat().st_size,
                 'note':'ELF section sizes only; target heap, total stack, timing and power remain unmeasured'}
        (out/'sections.json').write_text(json.dumps(metrics,indent=2)+'\n')
        print(f'Static RAM .data+.bss: {static_ram}/8192 bytes; flash image {metrics["flash_image_bytes"]}/1048576 bytes; no QSPI payload')


if __name__ == '__main__':
    main()
