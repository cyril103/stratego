#!/bin/sh
set -eu
# Keep saves/replays writable and persistent while package resources stay
# read-only. The engine's relative asset paths work through this symlink.
data_root=${XDG_DATA_HOME:-"$HOME/.local/share"}
case "$data_root" in /*) ;; *) data_root="$HOME/.local/share" ;; esac
game_data="$data_root/stratego3d"
mkdir -p "$game_data/saves" "$game_data/reports"
if [ ! -e "$game_data/assets" ] && [ ! -L "$game_data/assets" ]; then
    ln -s /usr/share/stratego3d/assets "$game_data/assets"
fi
if [ ! -d "$game_data/assets/models" ]; then
    printf '%s\n' "Stratego : ressources absentes dans $game_data/assets" >&2
    exit 1
fi
cd "$game_data"
exec /usr/lib/stratego3d/stratego "$@"
