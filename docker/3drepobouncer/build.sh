#!/bin/bash
# Builds the 3drepobouncer Docker image.
# All arguments are forwarded to docker build (e.g. --target, --build-arg, -t).
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

DOCKER_BUILDKIT=1 docker build "$@" \
    -f "${SCRIPT_DIR}/Dockerfile" \
    "${SCRIPT_DIR}/"
