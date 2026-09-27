#!/usr/bin/env python3
'''Check that every SDL symbol used inside an `#if *_VERSION_ATLEAST(x, y, z)`
block in src/ really exists in release x.y.z of that library.

A guard written too early compiles fine on newer SDL and silently drops the
code on older SDL, so it only breaks on a release in between: this catches it
without having to build against every release.

    python3 tools/check_version_guards.py [--cache DIR]

Headers for each guard version are downloaded from github.com/libsdl-org into
the cache directory (default: ~/.cache/pysdl2-headers) on first use. Code in an
`#else` branch of a version guard, or under a negated guard, is for *older*
releases and is not checked. Exit status 1 if anything is missing.
'''

import argparse
import io
import os
import pathlib
import re
import sys
import tarfile
import urllib.request

ROOT = pathlib.Path(__file__).resolve().parent.parent

# guard macro -> (repository, header name, identifier prefixes it covers)
LIBS = {
    'SDL_VERSION_ATLEAST':       ('SDL',       None,          ('SDL_',)),
    'SDL_IMAGE_VERSION_ATLEAST': ('SDL_image', 'SDL_image.h', ('IMG_',)),
    'SDL_TTF_VERSION_ATLEAST':   ('SDL_ttf',   'SDL_ttf.h',   ('TTF_',)),
    'SDL_MIXER_VERSION_ATLEAST': ('SDL_mixer', 'SDL_mixer.h', ('Mix_', 'MIX_', 'MUS_')),
}
# Guards may name development versions that were never tagged: check against
# the first release after them.
NEXT_RELEASE = {('SDL', (2, 0, 11)): (2, 0, 12)}
IGNORE = {'SDLCALL', 'SDL_VERSIONNUM'} | set(LIBS)

PASTE_MACRO = re.compile(r'#define\s+(\w+)\((\w+)\)(.*)')
GUARD = re.compile(r'(!?)\s*(SDL(?:_IMAGE|_TTF|_MIXER)?_VERSION_ATLEAST)\s*\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\)')
IDENT = re.compile(r'\b[A-Za-z_][A-Za-z0-9_]*\b')


def _tag(version):
    return 'release-%d.%d.%d' % version


def headers_text(repo, header, version, cache):
    '''All of `repo`'s public headers at `version`, as one string.'''
    version = NEXT_RELEASE.get((repo, version), version)
    target = cache / ('%s-%d.%d.%d.txt' % ((repo,) + version))
    if not target.exists():
        cache.mkdir(parents=True, exist_ok=True)
        if header is None:  # SDL itself: every include/*.h from the release tarball
            url = 'https://github.com/libsdl-org/%s/archive/refs/tags/%s.tar.gz' % (repo, _tag(version))
            data = urllib.request.urlopen(url).read()
            parts = []
            with tarfile.open(fileobj=io.BytesIO(data)) as tar:
                for member in tar.getmembers():
                    if re.search(r'/include/[^/]+\.h$', member.name):
                        parts.append(tar.extractfile(member).read().decode('utf-8', 'replace'))
            text = '\n'.join(parts)
        else:
            text = None
            for path in ('include/' + header, header):
                url = 'https://raw.githubusercontent.com/libsdl-org/%s/%s/%s' % (repo, _tag(version), path)
                try:
                    text = urllib.request.urlopen(url).read().decode('utf-8', 'replace')
                    break
                except OSError:
                    continue
            if text is None:
                raise SystemExit('could not fetch %s %s' % (repo, _tag(version)))
        target.write_text(text)
    return target.read_text()


def strip_code(line, in_comment):
    '''Drop comments and string literals; returns (code, still_in_comment).'''
    out = []
    i = 0
    while i < len(line):
        if in_comment:
            end = line.find('*/', i)
            if end < 0:
                return ''.join(out), True
            i, in_comment = end + 2, False
        elif line.startswith('/*', i):
            in_comment, i = True, i + 2
        elif line.startswith('//', i):
            break
        elif line[i] == '"':
            end = i + 1
            while end < len(line) and line[end] != '"':
                end += 2 if line[end] == '\\' else 1
            i = end + 1
        else:
            out.append(line[i])
            i += 1
    return ''.join(out), in_comment


def paste_macros(paths):
    '''Our own function-like macros that build identifiers by token pasting,
    e.g. `#define IMG_IS(FMT) ... IMG_is##FMT ...`: name -> [prefixes], so a
    use `IMG_IS(SVG)` can be checked as `IMG_isSVG`. Also returns every macro
    name we define, which are never library symbols.'''
    pasting, defined = {}, set()
    for path in paths:
        text = path.read_text().replace('\\\n', ' ')
        for line in text.splitlines():
            match = PASTE_MACRO.match(line.strip())
            if match:
                name, param, body = match.groups()
                prefixes = re.findall(r'(\w+)##%s\b' % re.escape(param), body)
                if prefixes:
                    pasting[name] = prefixes
            define = re.match(r'\s*#\s*define\s+(\w+)', line)
            if define:
                defined.add(define.group(1))
    return pasting, defined


def expand(code, pasting):
    '''Identifiers in `code`, plus what each pasting-macro use expands to.'''
    idents = set(IDENT.findall(code))
    for name, prefixes in pasting.items():
        for arg in re.findall(r'\b%s\s*\(\s*(\w+)\s*\)' % re.escape(name), code):
            idents.update(prefix + arg for prefix in prefixes)
    return idents


def scan(path, pasting):
    '''Yield (line_no, {guard_macro: version}, identifiers) for guarded code.'''
    frames = []  # (guards: {macro: version}, skip: bool) per #if nesting level
    in_comment = False
    for number, raw in enumerate(path.read_text().splitlines(), 1):
        code, in_comment = strip_code(raw, in_comment)
        directive = code.strip()
        if directive.startswith('#'):
            word = directive[1:].split(None, 1)[0] if len(directive) > 1 else ''
            rest = directive[1 + len(word):]
            if word in ('if', 'ifdef', 'ifndef'):
                guards, skip = {}, False
                if word == 'if':
                    for neg, macro, a, b, c in GUARD.findall(rest):
                        if neg:
                            skip = True  # branch for older releases
                        else:
                            guards[macro] = max(guards.get(macro, (0, 0, 0)), (int(a), int(b), int(c)))
                frames.append((guards, skip))
            elif word in ('else', 'elif') and frames:
                guards, skip = frames[-1]
                frames[-1] = ({}, skip or bool(guards))  # the other side of a version guard is older code
            elif word == 'endif' and frames:
                frames.pop()
            continue
        if not frames or any(skip for _, skip in frames):
            continue
        required = {}
        for guards, _ in frames:
            for macro, version in guards.items():
                required[macro] = max(required.get(macro, (0, 0, 0)), version)
        if required:
            yield number, required, expand(code, pasting)


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    parser.add_argument('--cache', default=os.path.expanduser('~/.cache/pysdl2-headers'))
    args = parser.parse_args()
    cache = pathlib.Path(args.cache)

    sources = sorted((ROOT / 'src').glob('*.c')) + sorted((ROOT / 'src').glob('*.h'))
    pasting, ours = paste_macros(sources)
    problems = checked = 0
    for path in sources:
        for number, required, idents in scan(path, pasting):
            for macro, version in required.items():
                repo, header, prefixes = LIBS[macro]
                names = {i for i in idents
                         if i.startswith(prefixes) and i not in IGNORE and i not in ours}
                if not names:
                    continue
                text = headers_text(repo, header, version, cache)
                for name in sorted(names):
                    checked += 1
                    if not re.search(r'\b%s\b' % re.escape(name), text):
                        problems += 1
                        print('%s:%d: %s is not in %s %d.%d.%d (guard %s)' % (
                            path.relative_to(ROOT), number, name, repo, *version, macro))
    print('%d guarded symbol uses checked, %d problem(s)' % (checked, problems))
    return 1 if problems else 0


if __name__ == '__main__':
    sys.exit(main())
