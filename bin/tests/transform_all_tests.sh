#!/bin/bash
#
# Convert all CON files in directory "tests" to YAML format.
# Also apply possible transformations due to the change in input source tree.
# The transformations have to be defined in the file
# "src/python/GeoMop/ModelEditor/resources/transformations/main.json".

echo "Transformation python scripts depends on packages: python3, python3-yaml, python3-markdown"
echo "Transformation list: src/python/GeoMop/ModelEditor/resources/transformations/flow123d.json"

# Relative path to "tests" directory from directory,
# where this script is placed
TESTS_DIR="../../tests"
# Relative path to "tests" directory from current/working directory
TESTS_DIR="${0%/*}/${TESTS_DIR}"

# Relative path to "importer.py" script from directory,
# where this script is placed
IMPORTER_PY="../../src/python/GeoMop/ModelEditor/importer.py"
# Relative path to "importer.py" script from current/working directory
IMPORTER_PY="${0%/*}/${IMPORTER_PY}"


for f in $TESTS_DIR/*/*.con; do
  echo "Processing $f";
  rm -f ${f%.con}.yaml;
  python3 $IMPORTER_PY --transformation-name flow123d --con_file $f;
done
