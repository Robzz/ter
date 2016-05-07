#!/bin/zsh
function archive_info () {
    if [[ ! -e pov_archive ]]; then
        echo "No archive file found, run a capture first"
        return 1
    fi
    make
    if [[ -e archive_dbg ]]; then
        rm -rf archive_dbg
    fi
    mkdir archive_dbg
    ./capture_archive_debug pov_archive archive_dbg > archive_dbg/povs.txt
}
