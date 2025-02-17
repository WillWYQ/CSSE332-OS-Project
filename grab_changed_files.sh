#!/bin/bash

# Check if enough arguments are provided
if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <old_commit> <new_commit> <output_directory>"
    exit 1
fi

OLD_COMMIT=$1
NEW_COMMIT=$2
OUTPUT_DIR=$3

# Detect if the system is macOS
IS_MACOS=false
if [[ "$(uname)" == "Darwin" ]]; then
    IS_MACOS=true
fi

# Create the output directory
mkdir -p "$OUTPUT_DIR"

# Get a list of changed files
CHANGED_FILES=$(git diff --name-only "$OLD_COMMIT" "$NEW_COMMIT")

# Copy changed files to the output directory while maintaining directory structure
for FILE in $CHANGED_FILES; do
    # Skip .DS_Store files
    if [[ "$FILE" == ".DS_Store" ]]; then
        continue
    fi

    DIR_PATH=$(dirname "$FILE")
    mkdir -p "$OUTPUT_DIR/$DIR_PATH"
    cp "$FILE" "$OUTPUT_DIR/$FILE"
done

# If macOS, remove any stray .DS_Store files in the copied directory
if $IS_MACOS; then
    find "$OUTPUT_DIR" -name ".DS_Store" -type f -delete
fi

echo "Changed files copied to $OUTPUT_DIR"
