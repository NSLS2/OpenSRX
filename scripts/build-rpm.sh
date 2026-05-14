#!/bin/bash
set -euo pipefail

mkdir -p SOURCES
rpmbuild -v --noclean \
    --define "_topdir $(pwd)" \
    --define "buildroot %{_topdir}/BUILDROOT" \
    --undefine=_disable_source_fetch \
    --define "_rpmfilename %%{NAME}-%%{VERSION}-%%{RELEASE}.%%{ARCH}.rpm" \
    -bb ./dist/*.spec
cp RPMS/*.rpm .
