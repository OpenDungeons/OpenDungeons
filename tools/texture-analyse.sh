#!/bin/sh

scan_meshes=false
scan_textures=false

if [ ! -d source ]; then
    echo "Please run this script for the root of the repository."
    exit 1
fi

if $scan_meshes; then
    if [ -z "$(command -v OgreXMLConverter)" ]; then
        echo "It appears that OgreXMLConverter is not in your path."
        echo "We won't search for materials matches in the mesh files."
        search_meshes=false
    else
        pushd models > /dev/null
        rm -rf xml && mkdir xml
        for mesh in *.mesh; do
            OgreXMLConverter $mesh xml/$mesh.xml &> /dev/null
            materials=$(cat xml/$mesh.xml | grep "material=" | cut -d \" -f2)
            for material in $materials; do
                echo -e "$mesh:\t$material"
            done
        done
        popd > /dev/null
    fi
fi

if $scan_textures; then
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
fi
