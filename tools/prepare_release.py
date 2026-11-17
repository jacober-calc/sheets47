#!/usr/bin/env python3
"""Prepare the local 0.1.0 review package, source archive, and validation record.

Requires successful current host, sanitizer and device builds. Does not publish,
commit, upload to hardware, download, clean, or delete files.
"""
import gzip
import hashlib
import json
from pathlib import Path
import shlex
import struct
import subprocess
import tarfile

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'dist/sheets47-0.1.0'


def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def current_build(mode):
    folder=ROOT/'build'/mode
    info=json.loads((folder/'build.json').read_text())
    headers=sorted((ROOT/'src').rglob('*.h'))+sorted((ROOT/'src').rglob('*.hpp'))+sorted((ROOT/'third_party').rglob('*.h'))
    header_hash=hashlib.sha256(b''.join(path.read_bytes() for path in headers)).hexdigest()
    if info['headers_sha256']!=header_hash:raise ValueError(f'{mode}: headers changed after build')
    for command in json.loads((folder/'compile_commands.json').read_text()):
        arguments=command['arguments'];source=ROOT/command['file'];obj=Path(arguments[-1])
        expected=hashlib.sha256(source.read_bytes()+header_hash.encode()+shlex.join(arguments).encode()).hexdigest()
        if obj.with_suffix(obj.suffix+'.sha256').read_text()!=expected:raise ValueError(f'{mode}: stale object for {source}')
    if mode!='device':
        last=(folder/'tests.log').read_text().strip().splitlines()[-1]
        if not last.startswith('PASS ') or ' checks in ' not in last:raise ValueError(f'{mode}: latest tests did not pass')
    return info


def main():
    subprocess.run(['python3','-B','tools/vendor_audit.py'],cwd=ROOT,check=True)
    builds={name:current_build(name) for name in ['host','host-sanitize','device']}
    binary=OUT/'sheets47.pg5';data=binary.read_bytes()
    if len(data)<84 or hashlib.sha1(data[:-20]).digest()!=data[-20:]:raise ValueError('PG5 integrity failed')
    magic,size,entry,cnr,version,qsize,qcrc=struct.unpack_from('<7I',data)
    if magic!=0xd377c0de or size!=len(data)-20 or cnr!=3 or version!=17 or qsize or qcrc:raise ValueError('Unexpected PG5 requirements')
    if not (entry&1 and 0x080a0000<=entry-1<0x080a0000+size):raise ValueError('Invalid program entry')
    if data[28:44].split(b'\0')[0]!=b'sheets47' or data[44:60].split(b'\0')[0]!=b'0.1.0':raise ValueError('Program identity mismatch')
    if struct.unpack_from('<I',data,60)[0]!=0x32334d44:raise ValueError('Program must require the DM32 system keymap')
    raw_image=bytearray((ROOT/'build/device/sheets47.bin').read_bytes())
    struct.pack_into('<I',raw_image,4,len(raw_image))
    if data[:-20]!=bytes(raw_image):
        raise ValueError('PG5 does not match current raw image')
    sources={}
    for top in ['src','tests','tools','third_party']:
        for path in sorted((ROOT/top).rglob('*')):
            if path.is_file() and '__pycache__' not in path.parts:sources[str(path.relative_to(ROOT))]=digest(path)
    for name in ['Makefile','LICENSE','NOTICE.md']:sources[name]=digest(ROOT/name)
    technical_source_hash=hashlib.sha256(json.dumps(sources,sort_keys=True).encode()).hexdigest()
    sections=json.loads((ROOT/'build/device/sections.json').read_text())
    host=json.loads((ROOT/'build/host/validation.json').read_text());asan=json.loads((ROOT/'build/host-sanitize/validation.json').read_text())
    roundtrip=json.loads((ROOT/'build/host/roundtrip.json').read_text())
    if roundtrip['status']!='passed' or roundtrip['host_binary_sha256']!=digest(ROOT/'build/host/sheets47-tests'):raise ValueError('Current real-filesystem roundtrip has not passed')
    stack=[]
    for path in (ROOT/'build/device').glob('*.su'):
        for line in path.read_text().splitlines():
            parts=line.split('\t')
            if len(parts)>=3 and parts[1].isdigit():stack.append({'bytes':int(parts[1]),'function':parts[0],'kind':parts[2]})
    stack.sort(key=lambda item:item['bytes'],reverse=True)
    record={
        'date':'2026-11-15','program':'sheets47','version':'0.1.0','status':'host-tested and cross-linked; storage/menu repair awaits DM32 retest',
        'prior_build_device_report':json.loads((ROOT/'docs/validation/2026-11-15-dm32-report.json').read_text()),
        'base_git_revision':subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),
        'technical_source_set_sha256':technical_source_hash,'source_sha256':sources,
        'program_bytes':len(data),'program_sha256':digest(binary),'interface':f'{cnr}.{version}',
        'required_keymap':'DM32','required_keymap_id':'0x32334d44',
        'sections':sections,'builds':builds,'host':host,'sanitizers':asan,'host_filesystem_roundtrip':roundtrip,
        'largest_compiled_function_frame':stack[0] if stack else None,
        'target_heap':'not measured','target_stack_high_water':'not measured','hardware_timing_and_power':'not measured',
    }
    (ROOT/'docs/validation').mkdir(exist_ok=True)
    (ROOT/'docs/validation/0.1.0.json').write_text(json.dumps(record,indent=2)+'\n')
    summary=f'''# sheets47 0.1.0 validation

Recorded 15 November 2026. Version 0.1.0 on `dev`, including the storage/menu repair.
**Status: host-tested and cross-linked; this replacement binary awaits DM32 retest.**

The user ran the original build on DM32: the worksheet/demo worked, but Save and
system-menu access failed. The [repair notes](changes/0.1.0-storage-menu-fix.md)
record the causes, changes, and retest sequence. Installed firmware/keymap and
the exact original device binary hash were not supplied. Previous hardware
feedback does not validate this replacement binary.

## Artifact and platform

- Program: `sheets47`; metadata version `0.1.0`.
- Artifact: `dist/sheets47-0.1.0/sheets47.pg5`, {len(data):,} bytes.
- SHA-256: `{digest(binary)}`.
- DMCP5 interface requirement: {cnr}.{version}, from the pinned official SDK 3.57.
- Required system keymap: DM32 (`0x32334d44`); the previous wildcard is removed.
- No QSPI payload, custom USB stack, serial console, or firmware update.
- ARM compiler: {builds['device']['compiler']}.
- Host compiler: {builds['host']['compiler']}.
- Exact dependency revisions, original checksums and seven Spreader/one helper
  source patches are recorded in `third_party/`; `tools/vendor_audit.py` passes.

## Link measurements

| Item | Bytes | What this proves |
| --- | ---: | --- |
| Flash image before loader digest | {sections['flash_image_bytes']:,} / 1,048,576 | Fits the selected SDK program region |
| `.data` | {sections['sections']['.data']:,} | Initialized application RAM section |
| `.bss` | {sections['sections']['.bss']:,} | Zero-initialized application RAM section |
| `.data + .bss` | {sections['static_ram_bytes']:,} / 8,192 | Fits the selected static RAM region |
| Largest reported compiled function frame | {stack[0]['bytes'] if stack else 0:,} | Compiler report only; not total call-stack use |

The ordinary `size` summary classifies some flash-resident tables as “data”.
The table above uses actual section placement (`size -A` and the link map),
not that summary. Heap use, largest free target block, stack high-water, target
response time, and battery current/life have **not** been measured.
The INFO screen exposes current target free/largest heap for future trials.

## Host evidence

- {host['checks']} checks passed with the actual patched Spreader backend.
- {host['allocation_failure_points']} successive C++ allocation failure points and
  {host['c_allocation_failure_points']} project `malloc`/`realloc` failure points
  passed for candidate cell edits, preserving old contents and continued use.
- AddressSanitizer, UndefinedBehaviorSanitizer and LeakSanitizer passed the
  same {asan['checks']} checks. The sanitizer run required execution outside the
  sandbox because LeakSanitizer cannot inspect its threads under tracing.
- Formula results, dependency updates, invalid syntax, division errors, cycles,
  relative/absolute references, captured clipboard contents, insert/delete/fill,
  clustered/scattered 256-cell sheets, and forward/reverse 128-formula chains
  are exercised. Whole-row/column ranges and unsupported arrays are rejected.
- Save/load preserves formulas, formats and viewport; corrupt latest generations
  fall back. Partial data/header writes, unavailable storage, and a failed Save
  after recovery from a corrupt newer generation preserve a usable prior save.
- UI tests exercise commit/cancel, reference insertion into the original edited
  cell, letter palette, range mode, unchanged-frame zero updates, partial
  navigation updates, and forced redraw after simulated resume.
- The actual DMCP5 file/input/menu adapters also run against SDK declarations
  with stub firmware services: write-protected idle state, directory creation,
  scoped write enable, open/write/close failures and exact protection restore,
  device-adapter save/load, FILE/SAVE, SHIFT+0 with/without release, FILE/SETUP,
  failed-save continue/cancel, and successful retry. Menu entry checks that no
  file is open and write permission is restored. Unmapped presses clear SHIFT.
  These are adapter regression tests, not physical firmware or key-scan tests.
- A separate three-process test uses the real host filesystem: save the sample
  total 18.50, reopen and edit quantity B2 to obtain 23.50, save again, then
  reopen in another process and recover 23.50. This verifies the host FilePort,
  not the device filesystem.
- The complete normal host suite took {host['host_test_ms']:.2f} ms in this run;
  the sanitizer run took {asan['host_test_ms']:.2f} ms. These are test-suite times,
  not calculator response measurements or benchmark guarantees.
- Instrumented host C++ peak live allocation: {host['host_cpp_peak_live_bytes']:,}
  bytes across the suite. This excludes direct libc allocations, renderer memory
  allocated on the host test stack, runtime overhead and target ABI differences.
  It must not be presented as total calculator heap use.

## Integration findings and remedies

The upstream desktop tile configuration produced a roughly 22.5 MB host C++
peak during early failure testing. Three smaller tile levels and 32-unit pools
now support the bounded worksheet without changing its calculation algorithms.
The generated parser stack and allocating cell factories incorrectly declared
no-throw paths; those declarations were corrected. Scanner allocation failure
now propagates with owned-buffer cleanup. The first leak check found two
partially constructed tiles retained after failure; unconditional pointer
cleanup in the tile destructor fixed them. The earlier diagnostic is retained
locally in `build/qualification/asan-before-tile-cleanup-fix.log`.

ABS/SQRT are absent from this Spreader revision and are not offered. The nine
exposed functions are tested. All changes and before/after fingerprints are
reviewable in the dependency patch files; original license notices remain.

## Hardware work still required

Use public sample workbooks with a recoverable calculator backup. Record each
DM42n/DM32 model, firmware/interface, keymap/overlay, artifact hash, free/largest
heap, actual stack high-water, load/edit/save timing, and observed outcomes.
Verify every input/softkey, screen row updates, automatic and explicit off,
draft/committed-state resume, USB disk handoff, full-storage/failed-close behavior,
and controlled save interruption. The host suspend/resume tests cover the
portable state model, not the firmware power state machine. The optional Tk
viewer has not been manually exercised in a graphical desktop session here.

The agent did not upload to or modify a physical calculator. The user supplied
the limited initial DM32 observations above. No compatibility or durability
claim follows merely from this successful link. Binary, source, notices and
build evidence are prepared locally for review rather than published remotely.

See [the user guide](user-guide.md), [build instructions](building.md),
[actual application images](screenshots/README.md), and
[machine-readable evidence](validation/0.1.0.json).
'''
    (ROOT/'docs/validation-0.1.0.md').write_text(summary)
    for source,name in [(ROOT/'NOTICE.md','NOTICE.md'),(ROOT/'LICENSE','LICENSE'),(ROOT/'docs/user-guide.md','USER-GUIDE.md'),(ROOT/'docs/validation-0.1.0.md','VALIDATION.md')]:
        (OUT/name).write_bytes(source.read_bytes())
    documents=['building.md','user-guide.md','validation-0.1.0.md','validation/0.1.0.json','validation/2026-11-15-dm32-report.json','validation/2026-11-16-dm32-header.json','changes/0.1.0-storage-menu-fix.md','screenshots/README.md']
    documents += [str(path.relative_to(ROOT/'docs')) for path in (ROOT/'docs/screenshots').glob('*.png')]
    for name in documents:
        destination=OUT/'docs'/name;destination.parent.mkdir(parents=True,exist_ok=True)
        destination.write_bytes((ROOT/'docs'/name).read_bytes())
    (OUT/'VALIDATION.md').write_text(summary.replace('(user-guide.md)','(docs/user-guide.md)').replace('(building.md)','(docs/building.md)').replace('(screenshots/README.md)','(docs/screenshots/README.md)').replace('(validation/0.1.0.json)','(docs/validation/0.1.0.json)').replace('(changes/0.1.0-storage-menu-fix.md)','(docs/changes/0.1.0-storage-menu-fix.md)'))
    (OUT/'README.md').write_text('''# sheets47 0.1.0 review package

`sheets47.pg5` is the standalone DMCP5 application. Interface 3.17 is required.
The program header requires the DM32 system keymap (`0x32334d44`).
This storage/menu repair is host-tested and cross-linked, awaiting DM32 retest.

- [User guide](docs/user-guide.md)
- [Validation and measured link sizes](docs/validation-0.1.0.md)
- [Storage/menu fixes and DM32 retest](docs/changes/0.1.0-storage-menu-fix.md)
- [Actual application images](docs/screenshots/README.md)
- [Build instructions](docs/building.md)
- [Notices](NOTICE.md), LICENSE, and the supplied licenses/ directory

The matching project source is in sheets47-0.1.0-source.tar.gz. Paths mentioned
in the notices/build guide refer to that source tree. Compiler binaries and
private local resources are excluded. Download/unpack instructions reproduce
the ignored local compiler installation. release.json identifies the artifacts.
No tool in this package automatically flashes or changes calculator firmware.
''')
    notices=OUT/'licenses';notices.mkdir(exist_ok=True)
    for component,filename in [('spreader','LICENSE'),('sys_string','LICENSE'),('intrusive_shared_ptr','LICENSE.txt'),('dmcp5','LICENSE'),('arm-runtime','LICENSES.txt')]:
        (notices/(component+'.txt')).write_bytes((ROOT/'third_party'/component/filename).read_bytes())
    # Git's tracked + ordinary untracked set excludes .local, build, dist and
    # private resources according to .gitignore. Refuse symlinks/unsafe paths.
    entries=subprocess.check_output(['git','ls-files','--cached','--others','--exclude-standard','-z'],cwd=ROOT).decode().split('\0')
    archive=OUT/'sheets47-0.1.0-source.tar.gz'
    with archive.open('wb') as raw:
        with gzip.GzipFile(fileobj=raw,mode='wb',mtime=0,filename='') as compressed:
            with tarfile.open(fileobj=compressed,mode='w') as tar:
                for name in sorted(set(filter(None,entries))):
                    path=ROOT/name
                    if path.is_symlink() or '..' in Path(name).parts:raise ValueError('Unsafe source archive path')
                    if not path.is_file():continue
                    info=tar.gettarinfo(str(path),arcname='sheets47-0.1.0/'+name)
                    info.uid=info.gid=0;info.uname=info.gname='';info.mtime=0
                    with path.open('rb') as stream:tar.addfile(info,stream)
    (OUT/'release.json').write_text(json.dumps({'program':'sheets47','version':'0.1.0','build_note':'2026-11-15 storage/menu repair; awaits DM32 retest','hardware_tested':False,'program_sha256':digest(binary),'source_archive_sha256':digest(archive),'technical_source_set_sha256':technical_source_hash},indent=2)+'\n')
    print('Prepared',OUT.relative_to(ROOT),'and tracked validation evidence')


if __name__=='__main__':main()
