#!/bin/sh

# Command-line arguments handling

scan_meshes=false
scan_textures=false
while [[ $# -ge 1 ]]; do
    key="$1"
    case $key in
        --help|-h)
        echo "Usage: <script> [options]"
        echo -e "\nAvailable options:"
        echo -e "\t--help, -h\t\tThis usage explanation."
        echo -e "\t--all, -a\t\tRun all tests."
        echo -e "\t--scan-meshes, -m\tScan meshes to link them with materials."
        echo -e "\t--scan-textures, -t\tScan textures to link them with materials."
        echo ""
        shift
        ;;
        --all|-a)
        echo "=== Scanning both mesh and textures files ==="
        scan_meshes=true
        scan_textures=true
        shift
        ;;
        --scan-meshes|-m)
        echo "=== Scanning mesh files ==="
        scan_meshes=true
        shift
        ;;
        --scan-textures|-t)
        echo "=== Scanning texture files ==="
        scan_textures=true
        shift
        ;;
        *)
        echo "Unknown option, use --help for usage instructions."
        ;;
esac
shift
done

# Preliminary tests

if [ ! -d source ]; then
    echo "Please run this script for the root of the repository."
    exit 1
fi

if $scan_meshes && [ -z "$(command -v OgreXMLConverter)" ]; then
    echo "It appears that 'OgreXMLConverter' is not in your path."
    echo "Your distribution might provide it in its 'ogre' or 'ogre-tools' package."
    echo "We won't search for materials matches in the mesh files."
    scan_meshes=false
fi

if $scan_textures && [ -z "$(command -v ack)" ]; then
    echo "It appears that 'ack' is not in your path."
    echo "Your distribution might provide it in its 'ack' package."
    echo "We won't search for textures matches in the materials files."
    scan_textures=false
fi

tmpdir="$(pwd)/tmp"
rm -rf $tmpdir && mkdir $tmpdir

if $scan_meshes; then
    pushd models > /dev/null
    for mesh in *.mesh; do
        OgreXMLConverter $mesh $tmpdir/$mesh.xml &> /dev/null
        materials=$(cat $tmpdir/$mesh.xml | grep "material=" | cut -d \" -f2)
        for material in $materials; do
            echo -e "$mesh:\t$material"
        done
    done
    popd > /dev/null
fi

if $scan_textures; then
    pushd materials/textures > /dev/null
    ls * > $tmpdir/textures-list.txt
    popd > /dev/null

    for texture in $(cat $tmpdir/textures-list.txt)
    do
        echo "== Texture: $texture =="
        echo ". Files matching the filename:"
        ack -il --ignore-dir=tmp $texture
        for script in $(ack -il --ignore-dir=tmp $texture | grep materials/scripts)
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
            done
        done
        echo ""
    done
fi
