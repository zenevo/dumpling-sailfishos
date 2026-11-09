#!/bin/sh
# Apply the requested CPU mode. Run as root by cpu-mode.service
# Modes:
#   powersave   - capped frequencies, battery first
#   balanced    - interactive, small cores kept above 1.2 GHz
#   performance - full speed (all cores unlocked)
MODE=$(cat /home/defaultuser/.config/cpu-mode/mode 2>/dev/null | tr -d ' \n')
[ -z "$MODE" ] && MODE=balanced

LITTLE="0 1 2 3"
BIG="4 5 6 7"

# default (unlocked) maximum frequencies
LITTLE_MAX=1900800
BIG_MAX=2457600

set_cluster() {
    # $1: cpu list, $2: governor, $3: max, $4: min
    for i in $1; do
        c=/sys/devices/system/cpu/cpu$i/cpufreq
        [ -d "$c" ] || continue
        echo "$2" > "$c/scaling_governor" 2>/dev/null
        [ -n "$3" ] && echo "$3" > "$c/scaling_max_freq" 2>/dev/null
        [ -n "$4" ] && echo "$4" > "$c/scaling_min_freq" 2>/dev/null
    done
}

case "$MODE" in
    powersave)
        set_cluster "$LITTLE" interactive 1248000 300000
        set_cluster "$BIG"    interactive 1555200 300000
        ;;
    performance)
        set_cluster "$LITTLE" performance "$LITTLE_MAX" 1248000
        set_cluster "$BIG"    performance "$BIG_MAX"    300000
        ;;
    *)
        set_cluster "$LITTLE" interactive "$LITTLE_MAX" 1248000
        set_cluster "$BIG"    interactive "$BIG_MAX"    300000
        ;;
esac

exit 0
