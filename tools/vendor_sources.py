#!/usr/bin/env python3
"""Import only selected regular files from pinned, locally cached source archives.

No network, upstream build execution, symlinks, or cleanup. Refuses to overwrite
different files; local patches are recorded separately. Run once to establish a
new snapshot; downloads remain in .local/downloads. Does not install toolchains.
"""
import hashlib
import json
from pathlib import Path, PurePosixPath
import tarfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    lock = json.loads((ROOT / 'third_party/sources.lock.json').read_text())
    for dep in lock['dependencies']:
        archive = ROOT / '.local/downloads' / dep['archive']
        destination = ROOT / 'third_party' / dep['name']
        entries = []
        with tarfile.open(archive) as source:
            for member in source.getmembers():
                parts = PurePosixPath(member.name).parts
                if len(parts) < 2 or not member.isfile():
                    continue
                relative = PurePosixPath(*parts[1:])
                if relative.is_absolute() or '..' in relative.parts:
                    raise ValueError('Unsafe archive path')
                name = relative.as_posix()
                if not (name in dep['files'] or any(name.startswith(p) for p in dep['prefixes'])):
                    continue
                data = source.extractfile(member).read()
                output = destination / name
                if output.exists() and output.read_bytes() != data:
                    raise ValueError(f'Refusing to overwrite modified file: {output}')
                output.parent.mkdir(parents=True, exist_ok=True)
                if not output.exists():
                    output.write_bytes(data)
                entries.append({'path': name, 'sha256': hashlib.sha256(data).hexdigest()})
        manifest = {
            'repository': 'https://github.com/' + dep['repository'],
            'revision': dep['revision'],
            'archive_url': f"https://codeload.github.com/{dep['repository']}/tar.gz/{dep['revision']}",
            'archive_sha256': hashlib.sha256(archive.read_bytes()).hexdigest(),
            'files': sorted(entries, key=lambda e: e['path']),
        }
        destination.mkdir(parents=True, exist_ok=True)
        (destination / 'SNAPSHOT.json').write_text(json.dumps(manifest, indent=2) + '\n')
        notice = f"""# {dep['name']} upstream snapshot

Source: {manifest['repository']}
Revision: `{dep['revision']}`
Archive SHA-256: `{manifest['archive_sha256']}`

Exact imported files and original checksums are in SNAPSHOT.json. Selection is
defined by ../sources.lock.json and tools/vendor_sources.py. Tests, wrappers,
desktop interfaces, binaries, and unrelated data are excluded. Original license
files and per-file notices are retained. Upstream build scripts are reference
material only; the project build selects source files directly and performs no
network fetch or cleanup. Build-time patches, if any, are recorded in PATCHES.md.
"""
        (destination / 'UPSTREAM.md').write_text(notice)
        print(dep['name'], len(entries), 'files', manifest['archive_sha256'])


if __name__ == '__main__':
    main()
