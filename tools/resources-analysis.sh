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
        echo "Scanning both mesh and textures files (takes a while)..."
        scan_meshes=true
        scan_textures=true
        shift
        ;;
        --scan-meshes|-m)
        echo "Scanning mesh files..."
        scan_meshes=true
        shift
        ;;
        --scan-textures|-t)
        echo "Scanning texture files (takes a while)..."
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

# Actual analysis scripts

basedir="$(pwd)/tools"
# Using tools/tmp instead of tmp in case $basedir would be undefined,
# so that we don't clean the system's /tmp
tmpdir="$basedir/tmp"
rm -rf $tmpdir && mkdir $tmpdir

if $scan_meshes; then
    log_meshes="$basedir/meshes2materials.log"
    echo -e "[mesh filename]:\t[material name]\n" > $log_meshes

    for mesh in $(ls models/*.mesh | sed -e 's@models/@@'); do
        OgreXMLConverter models/$mesh $tmpdir/$mesh.xml &> /dev/null
        materials=$(cat $tmpdir/$mesh.xml | grep "material=" | cut -d \" -f2)
        for material in $materials; do
            echo -e "$mesh:\t$material" >> $log_meshes
        done
    done

    echo -e "Output written to $log_meshes\n"
fi

if $scan_textures; then
    log_textures="$basedir/textures2materials.log"
    log_unused="$basedir/textures_unused.log"
    echo -e "[texture filename]:\t[material name]\t[script filename]\n" > $log_textures
    echo -e "[texture filename]:\t[files referencing it]\n" > $log_unused

    for texture in $(ls materials/textures/* | sed -e 's@materials/textures/@@'); do
        matches=$(ack -il --ignore-dir=$basedir $texture)
        # echo transforms lists of words to a space-separated string
        scripts=$(echo $matches | tr ' ' '\n' | grep "materials/scripts")
        if [ -z "$(echo $scripts)" ]; then
            echo -e "$texture:\t$matches" >> $log_unused
        else
            for script in $scripts; do
                for word in $(cat $script | grep "^ *material " | sed -e 's/^ *material //'); do
                    # Skip the words which are not material names
                    if [ -n "$(echo $word | grep RTSS)" -o -n "$(echo $word | grep :)" ]; then
                        continue
                    fi
                    echo -e "$texture:\t$word\t$script" | sed -e 's@materials/scripts/@@' >> $log_textures
                done
            done
        fi
    done

    echo -e "Output written to $log_textures\nand $log_unused\n"
fi
