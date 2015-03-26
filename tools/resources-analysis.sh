#!/bin/sh

# Command-line arguments handling

scan_meshes=false
scan_textures=false
generate_tree=false
while [[ $# -ge 1 ]]; do
    key="$1"
    case $key in
        --help|-h)
        echo "Usage: <script> [options]"
        echo -e "\nAvailable options:"
        echo -e "\t--help, -h\t\tThis usage explanation."
        echo -e "\t--all, -a\tPerform all supported actions (equivalent to -m -t -g)."
        echo -e "\t--scan-meshes, -m\tScan meshes to link them with materials."
        echo -e "\t--scan-textures, -t\tScan textures to link them with materials."
        echo -e "\t--generate-tree, -g\t."
        echo ""
        ;;
        --all|-a)
        scan_meshes=true
        scan_textures=true
        generate_tree=true
        ;;
        --scan-meshes|-m)
        scan_meshes=true
        ;;
        --scan-textures|-t)
        scan_textures=true
        ;;
        --generate-tree|-g)
        generate_tree=true
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
tmpdir="$basedir/tmp"
if [ ! -d $tmpdir ]; then
    mkdir -p $tmpdir
fi

log_meshes="$basedir/meshes2materials.log"
log_meshes_unused="$basedir/meshes_unused.log"
log_textures="$basedir/textures2materials.log"
log_textures_unused="$basedir/textures_unused.log"

if $scan_meshes; then
    echo "Scanning mesh files..."
    echo -e "[mesh filename]:\t[material name]\n" > $log_meshes
    echo -e "[mesh filename]:\t[files referencing it]\n" > $log_meshes_unused

    for mesh in $(ls models/*.mesh | sed -e 's@models/@@'); do
        matches=$(ack -il --ignore-dir=tools --ignore-dir=materials \
                          --ignore-file=is:CREDITS ${mesh%.mesh} | tr '\n' ' ')
        if [ -z "$matches" ]; then
            echo -e "$mesh:\t$matches" >> $log_meshes_unused
        fi

        OgreXMLConverter -log /dev/null models/$mesh $tmpdir/$mesh.xml &> /dev/null
        materials=$(cat $tmpdir/$mesh.xml | grep "material=" | cut -d \" -f2)
        for material in $materials; do
            echo -e "$mesh:\t$material" >> $log_meshes
        done
    done

    echo -e "Output written to $log_meshes\nand $log_meshes_unused\n"
fi

if $scan_textures; then
    echo "Scanning texture files (takes a while)..."
    echo -e "[texture filename]:\t[material name]\t[script filename]\n" > $log_textures
    echo -e "[texture filename]:\t[files referencing it]\n" > $log_textures_unused

    for texture in $(ls materials/textures/* | sed -e 's@materials/textures/@@'); do
        matches=$(ack -il --ignore-dir=tools --ignore-file=is:CREDITS $texture)
        # echo transforms lists of words to a space-separated string
        scripts=$(echo $matches | tr ' ' '\n' | grep "materials/scripts")
        if [ -z "$(echo $scripts)" ]; then
            echo -e "$texture:\t$matches" >> $log_textures_unused
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

    echo -e "Output written to $log_textures\nand $log_textures_unused\n"
fi

if $generate_tree; then
    abort=false
    if [ ! -r $log_meshes ]; then
        abort=true
        echo -e "The meshes log could not be read in $log_meshes.\n" \
                "Please run the script first with the -m or -a argument."
    fi
    if [ ! -r $log_textures ]; then
        abort=true
        echo -e "The textures log could not be read in $log_textures.\n" \
                "Please run the script first with the -t or -a argument."
    fi
    if $abort; then
        exit 1
    fi

    echo "Establishing the relationship between meshes and textures via the materials..."
    log_tree="$basedir/meshes_tree.log"
    echo -e "[mesh filename]:\n - [material name] ([script filename])\n" \
            "    * [texture filename]\n" > $log_tree

    prevmesh=""
    log_meshes_t="$log_meshes.tailed"
    tail -n+3 $log_meshes > $log_meshes_t
    while read mline; do
        mesh=$(echo $mline | cut -d ":" -f1)
        if [ "$mesh" != "$prevmesh" ]; then
            echo $mesh >> $log_tree
            prevmesh=$mesh
        fi
        material=$(echo $mline | cut -d ":" -f2 | tr -d " ")
        script=$(grep -m1 $'\t'$material$'\t' $log_textures | cut -d $'\t' -f3)
        echo " - $material ($script)" >> $log_tree
        for texture in $(grep $'\t'$material$'\t' $log_textures | cut -d $'\t' -f1 | tr -d ":"); do
            echo "    * $texture" >> $log_tree
        done
    done < $log_meshes_t
fi
