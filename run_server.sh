# run 'chmod +x run_server.sh' to make this script executable

set -e # Exit on failure

exec ./build/server "$@"