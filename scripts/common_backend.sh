#!/bin/bash
# Common backend selection for dx8gl scripts
# Source this file and call parse_backend_option "$@"

# Function to parse backend option
parse_backend_option() {
    BACKEND="osmesa"  # Default to OSMesa
    
    while [ $# -gt 0 ]; do
        case "$1" in
            --backend=*)
                BACKEND="${1#*=}"
                # Normalize to lowercase
                BACKEND=$(echo "$BACKEND" | tr '[:upper:]' '[:lower:]')
                if [ "$BACKEND" != "osmesa" ] && [ "$BACKEND" != "egl" ]; then
                    echo "Error: Invalid backend '$BACKEND'. Use 'osmesa' or 'egl'."
                    exit 1
                fi
                shift
                ;;
            --help|-h)
                echo "Usage: $0 [--backend=osmesa|egl]"
                echo "  --backend=osmesa  Use OSMesa software rendering (default)"
                echo "  --backend=egl     Use EGL surfaceless context"
                exit 0
                ;;
            *)
                # Pass through other arguments
                break
                ;;
        esac
    done
    
    # Export backend for dx8gl
    export DX8GL_BACKEND=$BACKEND
    echo "Using $BACKEND backend"
}