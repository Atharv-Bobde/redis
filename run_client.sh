# run 'chmod +x run_client.sh' to make this script executable
set -e # Exit on failure

exec ./build/client "$@"