#!/bin/bash

while [[ $# -gt 0 ]]; do
  case $1 in
    -t|--rebuild-tools) rebuild_tools=1;;
    -g|--recook-gfx) recook_gfx=1;;
    -*|--*) echo "Unknown option $1"; exit 1;;
    *) id1path="$1";;
  esac
  shift
done

if [[ -z "$id1path" ]]; then
  echo "Specify path to id1 directory."
  exit 1
fi

if [[ ! -d ./tools/ ]]; then
  echo "Run this from the root quakepsx repo directory."
  exit 1
fi

# build tools if not present
if [[ -n $rebuild_tools ]]; then
  rm ./tools/bin/*
fi
if [[ ! -d ./tools/bin/ ]]; then
  mkdir -p ./tools/bin/ || exit 1
fi
if [[ ! -f ./tools/bin/bspconvpsx.exe || ! -f ./tools/bin/mkpics.exe ]]; then
  pushd ./tools
  make -C bspconvpsx/ || exit 1
  make -C mkpics/ || exit 1
  popd
fi

# build GFX.DAT if not present
if [[ ! -f ./id1psx/gfx.dat || -n "$recook_gfx" ]]; then
  ./tools/bin/mkpics.exe "$id1path" ./tools/cfg/id1 ./id1psx/gfx.dat || exit 1;
fi

# build maps
maps=("start" "e1m1" "e1m2" "e1m3" "e1m4" "e1m5" "e1m6" "e1m7" "e1m8")
# maps+=("e2m1" "e2m2" "e2m3" "e2m4" "e2m5" "e2m6" "e2m7")
# maps+=("e3m1" "e3m2" "e3m3" "e3m4" "e3m5" "e3m6" "e3m7")
# maps+=("e4m1" "e4m2" "e4m3" "e4m4" "e4m5" "e4m6" "e4m7" "e4m8")
# maps+=("end")
for m in "${maps[@]}"; do
  echo ":: converting $m"
  ./tools/bin/bspconvpsx.exe ./tools/cfg/id1 "$id1path" maps/$m.bsp ./id1psx/maps/$m.psb || exit 1
done
