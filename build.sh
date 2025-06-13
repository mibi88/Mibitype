#!/bin/sh

# Mibitype - A small library to load fonts.
# by Mibi88
#
# This software is licensed under the BSD-3-Clause license:
#
# Copyright 2024 Mibi88
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
# contributors may be used to endorse or promote products derived from this
# software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

src=("src/main.c" \
     "src/mibitype/reader.c" \
     "src/mibitype/glyph.c" \
     "src/mibitype/loader.c" \
     "src/mibitype/loaderlist.c" \
     "src/mibitype/font.c" \
     "src/mibitype/loaders/ttf.c" \
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

