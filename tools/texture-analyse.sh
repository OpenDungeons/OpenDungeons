#!/bin/sh

search_source=false

pushd materials/textures > /dev/null
ls * > ../../../textures-list.txt
popd > /dev/null

for texture in $(cat ../textures-list.txt)
do
    echo "== Texture: $texture =="
    echo ". Files matching the filename:"
    ack -il $texture
    for script in $(ack -il $texture | grep materials/scripts)
    do
        line=$(cat $script | grep "^ *material " | sed -e 's/^ *material //')
        echo -e ". Matched material definition:\t$line"
        for word in $line
        do
            if [ -n "$(echo $word | grep RTSS)" -o -n "$(echo $word | grep :)" ]
            then
                continue
            fi
            echo -e ". Extracted material name:\t$word"
            if $search_source
            then
                echo ". References to $word in the source code:"
                pushd source > /dev/null
                ack -il $word
                popd > /dev/null
                echo ""
            fi
        done
    done
    echo ""
done
