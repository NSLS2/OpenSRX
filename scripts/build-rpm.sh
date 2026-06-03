#!/bin/bash
set -euo pipefail

VERSION=$(grep -oP '^Version:\s+\K.*' dist/OpenSRX.spec)

mkdir -p SOURCES
tar czf "SOURCES/v${VERSION}.tar.gz" \
    --transform "s,^\.,OpenSRX-${VERSION}," \
    --exclude=BUILD --exclude=BUILDROOT --exclude=RPMS --exclude=SRPMS --exclude=SOURCES \
    --exclude=build --exclude=.git \
    .

rpmbuild -v --noclean \
    --define "_topdir $(pwd)" \
    --define "buildroot %{_topdir}/BUILDROOT" \
    --define "_rpmfilename %%{NAME}-%%{VERSION}-%%{RELEASE}.%%{ARCH}.rpm" \
    -bb ./dist/*.spec
cp RPMS/*.rpm .
