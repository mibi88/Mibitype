#!/bin/sh

src=("src/main.c" "src/mibitype/font.c" "src/mibitype/formats/ttf.c" \
     "src/render/render.c")
target="nes"
builddir="build"
warnings="-Wall -Wextra -Wpedantic"
incdirs=("src" "src/render")
libs=("SDL2")
objfiles=()
flags=("-g ")

output="mibitype"

run_cmd() {
    typeset cmd=$1
    echo " $ ${cmd}"
    $cmd
    typeset rc=$?
    if [ $rc != 0 ]; then
        echo "-- Build failed with return code ${rc}!"
        exit 1
    fi
}

flags+=$warnings
for dir in "${incdirs[@]}"
do
    flags+=("-I${dir}")
done

for lib in "${libs[@]}"
do
    flags+=("-l${lib}")
done

mkdir -p $builddir

for file in "${src[@]}"
do
    echo "-- Building ${file}..."
    base=$(basename $file)
    obj="${builddir}/${base}.o"
    cmd="cc -c ${file} -o ${obj} -ansi ${flags[@]}"
    run_cmd "${cmd}"
    objfiles+=($obj)
done

echo "-- Linking..."
outfile="${builddir}/${output}"
cmd="cc ${objfiles[@]} ${ldscript} -o ${outfile} ${flags[@]}"
run_cmd "${cmd}"

