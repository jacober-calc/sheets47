#!/usr/bin/env python3
"""Verify imported source hashes; --record-patches writes reviewable local diffs.

Recording requires the original archive cache. Verification requires only the
tracked snapshots and change records, and never downloads or deletes anything.
"""
import argparse
import difflib
import hashlib
import json
from pathlib import Path
import tarfile

ROOT=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--record-patches',action='store_true');args=parser.parse_args()
    lock=json.loads((ROOT/'third_party/sources.lock.json').read_text())
    for dep in lock['dependencies']:
        folder=ROOT/'third_party'/dep['name'];snapshot=json.loads((folder/'SNAPSHOT.json').read_text())
        if args.record_patches:
            archive=ROOT/'.local/downloads'/dep['archive']
            if hashlib.sha256(archive.read_bytes()).hexdigest()!=snapshot['archive_sha256']:raise ValueError('Original archive checksum mismatch')
            originals={}
            with tarfile.open(archive) as tar:
                for member in tar.getmembers():
                    if member.isfile():originals[member.name.partition('/')[2]]=tar.extractfile(member).read()
            changes=[];patch=[]
            for entry in snapshot['files']:
                name=entry['path'];original=originals[name];current=(folder/name).read_bytes()
                if hashlib.sha256(original).hexdigest()!=entry['sha256']:raise ValueError('Snapshot/original mismatch')
                if current!=original:
                    changes.append({'path':name,'original_sha256':entry['sha256'],'local_sha256':hashlib.sha256(current).hexdigest()})
                    patch.extend(difflib.unified_diff(original.decode().splitlines(True),current.decode().splitlines(True),fromfile='a/'+name,tofile='b/'+name))
            (folder/'LOCAL_CHANGES.json').write_text(json.dumps(changes,indent=2)+'\n')
            (folder/'LOCAL.patch').write_text(''.join(patch))
        changes=json.loads((folder/'LOCAL_CHANGES.json').read_text())
        expected={entry['path']:entry['sha256'] for entry in snapshot['files']}
        for entry in changes:
            if expected[entry['path']]!=entry['original_sha256']:raise ValueError('Patch provenance mismatch')
            expected[entry['path']]=entry['local_sha256']
        for name,digest in expected.items():
            if hashlib.sha256((folder/name).read_bytes()).hexdigest()!=digest:raise ValueError(f'Unexpected change: {dep["name"]}/{name}')
        print(f'{dep["name"]}: {len(expected)} imported files verified; {len(changes)} documented source patches')


if __name__=='__main__':main()
