#!/usr/bin/env bash
set -e

#
# Helper script to compile an already transpile model. The interactive execution
# model is used. That is, all non-determinism is resolved by user input.
#
# Usage: <path_to_script> <model_dir>
# * model_dir: positional argument; a model transpiled by human_readable_cmodel
#

#

# Ensures a model was passed, and that the model is valid.
if [[ $# -lt 1 ]]; then
        echo >&2 "Error: A transpiled model is required as the first argument."
        exit 1;
fi
MODEL_DIR=$1
if [[ "${MODEL_DIR:0:1}" != "/" ]]; then
	MODEL_DIR=`pwd`/"${MODEL_DIR}"
fi
if [[ ! "${MODEL_DIR}" || ! -d "${MODEL_DIR}" ]]; then
        echo >&2 "Error: Invalid output directory: ${MODEL_DIR}."
        exit 1;
fi

gcc -c "${MODEL_DIR}"/cmodel.c -I"${MODEL_DIR}" -o "${MODEL_DIR}"/cmodel.o
gcc -o "${MODEL_DIR}"/icmodel "${MODEL_DIR}"/cmodel.o -L"${MODEL_DIR}" -lverify_interactive

