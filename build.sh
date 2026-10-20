#!/bin/sh
# Build Gods from your own copy of the game:
#     ./build.sh "Gods (19xx)(Krisalis)(Disc 1 of 2).adf" "Gods (19xx)(Krisalis)(Disc 2 of 2).adf"
# then: cd recomp && ./gods
# The steps are recipe.json's "build", run by SuperSpider's tools/weave.py.
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
SUPERSPIDER="${SUPERSPIDER:-$HOME/stuff/superspider}"
[ $# -gt 0 ] || { echo "usage: $0 DISC1.adf DISC2.adf" >&2; exit 2; }
(cd "$SUPERSPIDER" && dune build)
WEAVE_RECIPES="$HERE" exec python3 -I "$SUPERSPIDER/tools/weave.py" build "$@"
