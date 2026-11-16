#!/usr/bin/env python3
"""Planted-defect test of tools/twin_fold_check.py.

A correct merge of two twins (declared in different orders) must MATCH both copies; a
merge whose conditional arms are swapped, one with a token changed outside the
conditionals, and one that drops a statement must each MISMATCH; a missing function must
fail. Fixtures are written to a temporary directory, so the test needs no git history.
"""
import os
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


def run(tmp, merged, name='PackRow'):
    b, c = os.path.join(tmp, 'base.c'), os.path.join(tmp, 'cand.c')
    with open(b, 'w') as f:
        f.write(BASE)
    with open(c, 'w') as f:
        f.write(merged)
    r = subprocess.run([sys.executable, os.path.join(HERE, 'twin_fold_check.py'), b, c,
                        '--merged', name, '--copy', 'PackRowLSB=PackLSB',
                        '--copy', 'PackRowMSB=PackMSB'], capture_output=True, text=True)
    return r.returncode, r.stdout


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
    return fail


if __name__ == '__main__':
    sys.exit(main())
