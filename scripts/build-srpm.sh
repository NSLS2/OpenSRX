#!/bin/bash
set -euo pipefail

mkdir -p SOURCES
rpmbuild -v --noclean \
    --define "_topdir $(pwd)" \
    --define "buildroot %{_topdir}/BUILDROOT" \
    --undefine=_disable_source_fetch \
    --define "_srpmfilename %%{NAME}-%%{VERSION}-%%{RELEASE}.%%{ARCH}.src.rpm" \
    -bs ./dist/*.spec
cp SRPMS/*.src.rpm .
