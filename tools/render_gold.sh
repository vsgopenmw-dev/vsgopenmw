#!/bin/bash
# render_gold.sh
# --------------
# vsg-side "gold render" recipe. A/B baseline against stock upstream at
# ../openmw/tools/render_gold.sh — same knobs, same target scene.
#
# Scene: START_CELL exterior, default view direction. Uses --skip-menu with
# no --new-game, no save → boot.hpp routes into bypassNewGame(START_CELL),
# which teleports the player to that cell without an intro sequence.
# The engine version doesn't matter (works on 0.49 vsg and 0.50 stock).
#
# We stopped using a real save because Bob_Bitchen/Quicksave is 0.52-era
# and the fork's 0.49 engine refuses it (newer-version guard).

set -u

# --- Paths (all absolute so we can be cd'd anywhere on entry) ---
OPENMW_BIN="/Users/bret.curtis/Workspace/private/vsgopenmw/build/OpenMW.app/Contents/MacOS/openmw"
START_CELL="${START_CELL:-Balmora}"
CONFIG_DIR="$HOME/Library/Preferences/openmw-vsg"
USER_DATA="$HOME/Library/Application Support/openmw-vsg"
SHOT_DIR="$USER_DATA/screenshots"

# --- Knobs ---
# Frame counting starts once state == Running (i.e. after cell has loaded).
# 120 frames ~= 2 s of rendering; enough for terrain/water to settle.
SHOT_FRAME=120
QUIT_FRAME=150
# Hard wall-clock kill regardless of what happens in-engine.
WALL_TIMEOUT_SECONDS=20

# --- Sanity ---
[[ -x "$OPENMW_BIN" ]] || { echo "openmw binary missing: $OPENMW_BIN"; exit 1; }
[[ -f "$CONFIG_DIR/openmw.cfg" ]] || {
    echo "seeding $CONFIG_DIR/openmw.cfg from user's real config"
    mkdir -p "$CONFIG_DIR"
    cp "$HOME/Library/Preferences/openmw/openmw.cfg" "$CONFIG_DIR/openmw.cfg"
}
mkdir -p "$SHOT_DIR"

# --- Run ---
# The openmw macOS bundle looks up its Resources dir CWD-relative, so we
# must run from inside the .app/Contents/MacOS directory.
cd "$(dirname "$OPENMW_BIN")"

"$OPENMW_BIN" \
    --config      "$CONFIG_DIR" \
    --user-data   "$USER_DATA" \
    --replace=config \
    --skip-menu \
    --start "$START_CELL" \
    --auto-screenshot="$SHOT_FRAME" \
    --auto-quit="$QUIT_FRAME" \
    > /tmp/openmw-vsg-gold.log 2>&1 &
PID=$!

# Hard wall-clock kill in the background.
( sleep "$WALL_TIMEOUT_SECONDS" && kill -TERM "$PID" 2>/dev/null && \
  sleep 2 && kill -KILL "$PID" 2>/dev/null ) &
KILLER=$!

wait "$PID" 2>/dev/null
RC=$?
kill "$KILLER" 2>/dev/null

# --- Report ---
LATEST=$(ls -t "$SHOT_DIR"/*.png 2>/dev/null | head -n 1 || true)
if [[ -n "${LATEST:-}" ]]; then
    echo "vsg gold render: $LATEST"
    echo "log:             /tmp/openmw-vsg-gold.log"
    exit 0
else
    echo "no screenshot produced; see /tmp/openmw-vsg-gold.log (exit=$RC)"
    tail -20 /tmp/openmw-vsg-gold.log
    exit 1
fi
