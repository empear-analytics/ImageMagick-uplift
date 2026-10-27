#!/bin/bash
# On ERDC, from 2026-10-26: fresh mutation data for the files refactored since
# 2026-10-24, so the readiness gate sees today's code instead of reports whose line
# numbers predate the campaign. Copy it to ~/imagemagick-mutation/setup/ and run it
# under the step7 lock:
#
#   nohup flock ~/imagemagick-mutation/setup/step7.lock ~/imagemagick-mutation/setup/night3.sh \
#     >> ~/imagemagick-mutation/setup/step7.log 2>&1 &
#
# 1. Check out the campaign tip (campaign/M99 while it is open, else main) and
#    rebuild the indexes. casemap.py and linecov.py both: a stale line profile makes
#    llvm-cov drop every changed function, and mutate.py then finds no mutants in it.
# 2. One Mull build of the files, default operators.
# 3. Per file, every mutant, capped at 200 cases (mutation-fresh-<file>.json), most
#    refactored files first; log.c leads for TranslateEvent. Pushed every fifth file.
# A file is skipped once its report exists, so the script can be started again; it
# will not finish in one night, and the next night simply resumes it.
SETUP=~/imagemagick-mutation/setup
cd ~/imagemagick-mutation/ImageMagick && . $SETUP/sysroot.env
export ORACLE_JOBS=16
W=build-oracle/work
FILES="log morphology geometry enhance visual-effects segment identify property image fx draw color statistic distort xml-tree profile constitute transform quantize montage cache pixel attribute option mime feature composite type delegate compress threshold string policy list linked-list histogram compare channel token splay-tree blob annotate vision utility signature resource matrix layer effect stream resample decorate colorspace shear registry random quantum paint magick locale gem exception colormap cipher semaphore magic fourier distribute-cache configure coder"
push() { $SETUP/push-results.sh > /dev/null 2>&1 && echo "$(date +%H:%M) pushed" || echo "$(date +%H:%M) push failed"; }

echo "== night3 $(date): step 1, checkout and indexes"
git fetch -q origin
REF=origin/campaign/M99
git rev-parse -q --verify $REF > /dev/null || REF=origin/main
if [ "$(git rev-parse HEAD)" != "$(git rev-parse $REF)" ] || [ ! -f build-oracle/linecov-night3.log ]; then
  git checkout -q --detach $REF && git log --oneline -1
  python3 -u tools/oracle/casemap.py > build-oracle/casemap-night3.log 2>&1 && \
    python3 -u tools/oracle/linecov.py > build-oracle/linecov-night3.log 2>&1 || { echo "indexes failed"; exit 1; }
  rm -rf build-oracle/mull-fresh
fi
echo "$(date +%H:%M) indexes at $(git rev-parse --short HEAD)"

echo "== night3 $(date): step 2, Mull build"
RX="MagickCore/($(echo $FILES | tr ' ' '|'))\\.c\$"
BIN=build-oracle/mull-fresh/utilities/magick
[ -x $BIN ] || tools/oracle/build.sh mull "$RX" fresh 2>&1 | tail -1
[ -x $BIN ] || { echo "$(date +%H:%M) mull build failed, see build-oracle/mull-fresh/make.log"; exit 1; }

echo "== night3 $(date): step 3, mutation per file"
n=0
for f in $FILES; do
  [ -f $W/mutation-fresh-$f.json ] && continue
  ( ulimit -v 4194304; exec python3 -u tools/oracle/mutate.py --file "MagickCore/$f\\.c\$" --bin $BIN --name fresh-$f \
    --max-cases 200 ) > build-oracle/fresh-$f.log 2>&1
  echo "$(date +%H:%M) fresh $f: $(grep -E 'killed [0-9]' build-oracle/fresh-$f.log || echo failed, see build-oracle/fresh-$f.log)"
  n=$((n+1)); [ $((n % 5)) = 0 ] && push
done
push
echo "== night3 done $(date)"
