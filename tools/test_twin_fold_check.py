#!/usr/bin/env python3
"""Planted-defect test of tools/twin_fold_check.py.

A correct merge of two twins (declared in different orders) must MATCH both copies; a
merge whose conditional arms are swapped, one with a token changed outside the
conditionals, and one that drops a statement must each MISMATCH; a missing function must
fail. A copy that names a role differently MISMATCHes until --rename maps the name. Fixtures are written to a temporary directory, so the test needs no git history.
"""
import os
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))

BASE = r'''
static unsigned char *PackRowLSB(const Image *canvas,
  const unsigned long *pixels,const Quantum *p,unsigned char *q)
{
  unsigned int
    nibble;

  int
    x;

  nibble=0;
  for (x=0; x < (int) canvas->columns; x++)
  {
    switch (nibble)
    {
      case 0: { *q=(unsigned char) pixels[x]; nibble++; break; }
      case 1: { *q|=(unsigned char) (pixels[x] << 4); q++; nibble=0; break; }
    }
    p+=(ptrdiff_t) GetPixelChannels(canvas);
  }
  return(q);
}

static unsigned char *PackRowMSB(const Image *canvas,
  const unsigned long *pixels,const Quantum *p,unsigned char *q)
{
  int
    x;

  unsigned int
    nibble;

  nibble=0;
  for (x=0; x < (int) canvas->columns; x++)
  {
    switch (nibble)
    {
      case 0: { *q=(unsigned char) (pixels[x] << 4); nibble++; break; }
      case 1: { *q|=(unsigned char) pixels[x]; q++; nibble=0; break; }
    }
    p+=(ptrdiff_t) GetPixelChannels(canvas);
  }
  return(q);
}
'''

MERGED = r'''
static inline unsigned char *PackRow(const PackRowInput *row,const Quantum *p,
  unsigned char *q)
{
  const PackOrder
    side = row->side;

  const Image
    *const canvas = row->canvas;

  const unsigned long
    *const pixels = row->pixels;

  unsigned int
    nibble;

  int
    x;

  nibble=0;
  for (x=0; x < (int) canvas->columns; x++)
  {
    switch (nibble)
    {
      case 0: { *q=((side == PackLSB) ? (unsigned char) pixels[x] :
        (unsigned char) (pixels[x] << 4)); nibble++; break; }
      case 1: { *q|=((side == PackLSB) ? (unsigned char) (pixels[x] << 4) :
        (unsigned char) pixels[x]); q++; nibble=0; break; }
    }
    p+=(ptrdiff_t) GetPixelChannels(canvas);
  }
  return(q);
}
'''


DRIVERS = r'''
static void DriveLSB(const Source *source,unsigned char *q)
{
  const Row
    row = { PackLSB, source->canvas };

  q=PackRow(&row,q);
}

static void DriveMSB(const Source *source,unsigned char *q)
{
  const Row
    row = { PackMSB, source->canvas };

  q=PackRow(&row,q);
}
'''

DRIVER = r'''
static inline void Drive(const Source *source,const PackOrder side,
  unsigned char *q)
{
  const Row
    row = { side, source->canvas };

  q=PackRow(&row,q);
}
'''


def run(tmp, merged, name='PackRow', base=BASE, copies=('PackRowLSB', 'PackRowMSB'),
        extra=()):
    b, c = os.path.join(tmp, 'base.c'), os.path.join(tmp, 'cand.c')
    with open(b, 'w') as f:
        f.write(base)
    with open(c, 'w') as f:
        f.write(merged)
    r = subprocess.run([sys.executable, os.path.join(HERE, 'twin_fold_check.py'), b, c,
                        '--merged', name, '--copy', copies[0] + '=PackLSB',
                        '--copy', copies[1] + '=PackMSB'] + list(extra),
                       capture_output=True, text=True)
    return r.returncode, r.stdout


def test_rename(tmp):
    """--rename: a copy that names a role differently matches only once renamed."""
    fail = 0
    at = BASE.index('static unsigned char *PackRowMSB(')
    renamed = BASE[:at] + re.sub(r'\bcanvas\b', 'image', BASE[at:])
    for label, extra, want, matches in (
            ('role renamed in one copy, no --rename', (), 1, 1),
            ('role renamed in one copy, --rename',
             ('--rename', 'PackRowMSB:image=canvas'), 0, 2),
            ('wrong --rename', ('--rename', 'PackRowMSB:image=pixels'), 1, 1)):
        st, out = run(tmp, MERGED, base=renamed, extra=extra)
        ok = st == want and out.count('MATCH ') - out.count('MISMATCH ') == matches
        print('%s %s (exit %d)' % ('ok  ' if ok else 'FAIL', label, st))
        if not ok:
            print(out)
            fail = 1
    return fail


def main():
    fail = 0
    swapped = MERGED.replace('? (unsigned char) pixels[x] :\n        (unsigned char) (pixels[x] << 4))',
                             '? (unsigned char) (pixels[x] << 4) :\n        (unsigned char) pixels[x])')
    assert swapped != MERGED
    changed = MERGED.replace('nibble=0; break;', 'nibble=1; break;')
    dropped = MERGED.replace('    p+=(ptrdiff_t) GetPixelChannels(canvas);\n', '')
    cases = [('correct merge', MERGED, 'PackRow', 0, 2),
             ('swapped arms', swapped, 'PackRow', 1, 0),
             ('changed token', changed, 'PackRow', 1, 0),
             ('dropped statement', dropped, 'PackRow', 1, 0),
             ('missing function', MERGED, 'NoSuchRow', 1, 0)]
    with tempfile.TemporaryDirectory() as tmp:
        for label, text, name, want, matches in cases:
            st, out = run(tmp, text, name)
            ok = st == want and out.count('MATCH ') - out.count('MISMATCH ') == matches
            print('%s %s (exit %d)' % ('ok  ' if ok else 'FAIL', label, st))
            if not ok:
                print(out)
                fail = 1
        # selector as a parameter: the constant run is written as the selector
        for label, text, want, matches in (
                ('selector parameter', DRIVER, 0, 2),
                ('constant kept in place of the selector',
                 DRIVER.replace('{ side,', '{ PackLSB,'), 1, 1)):
            st, out = run(tmp, text, 'Drive', DRIVERS, ('DriveLSB', 'DriveMSB'))
            ok = st == want and out.count('MATCH ') - out.count('MISMATCH ') == matches
            print('%s %s (exit %d)' % ('ok  ' if ok else 'FAIL', label, st))
            if not ok:
                print(out)
                fail = 1
        fail |= test_rename(tmp)
    return fail


if __name__ == '__main__':
    sys.exit(main())
