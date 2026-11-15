#!/usr/bin/env python3
"""Exercise real host filesystem persistence across three application processes.

Uses append-only data in build/host/roundtrip-SHEETS47; retains all generations
and logs. Requires the successful host build. No files are removed.
"""
import hashlib
import json
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'build/host'
BINARY=OUT/'sheets47-tests'
DISK=OUT/'roundtrip-SHEETS47'


def run(number,commands):
    result=subprocess.run([str(BINARY),'--repl',str(DISK)],cwd=ROOT,input='\n'.join(commands+['quit'])+'\n',capture_output=True,text=True,check=True)
    (OUT/f'roundtrip-{number}.log').write_text(result.stdout+result.stderr)
    return result.stdout


def main():
    run(1,['key F5','key F4','key F5','key F1'])
    opened=run(2,['value D6','key LEFT','key LEFT','key ENTER','key BSP','key 5','key ENTER','value D6','key F5','key F1'])
    if '\n18.50\n' not in opened or '\n23.50\n' not in opened:raise ValueError('Saved/recalculated totals differ')
    reopened=run(3,['value D6','frame '+str(OUT/'reopened.pbm')])
    if '\n23.50\n' not in reopened:raise ValueError('Edited formula result was not restored')
    record={'status':'passed','sessions':3,'initial_total':'18.50','edited_and_reopened_total':'23.50',
            'host_binary_sha256':hashlib.sha256(BINARY.read_bytes()).hexdigest(),
            'note':'Real host filesystem only; not DMCP5 filesystem or power-loss evidence'}
    (OUT/'roundtrip.json').write_text(json.dumps(record,indent=2)+'\n')
    print('PASS three-process real-filesystem roundtrip: 18.50 -> 23.50 -> reopened 23.50')


if __name__=='__main__':main()
