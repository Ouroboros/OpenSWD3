#!/usr/bin/env bash
# Builds the Ubuntu noble i386 multilib sysroot used by the Linux (i386, static) preset.
# Packages are downloaded with apt-get download (falling back to the archive pool when the
# local index points at a removed version) and unpacked without touching the system dpkg state.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/common.sh"

debs="${SWD3_ROOT}/tmp/i386-debs"
mkdir -p "${debs}"
cd "${debs}"

apt_pkgs=(libgcc-13-dev lib32gcc-13-dev libstdc++-13-dev lib32stdc++-13-dev lib32gcc-s1 lib32stdc++6
          libcrypt-dev lib32asan8 lib32ubsan1 lib32atomic1 lib32quadmath0 lib32itm1)
apt-get -o APT::Sandbox::User=root download "${apt_pkgs[@]}"

pool_fetch() {
    local dir="$1" pattern="$2"
    local index file
    index="$(curl -s -f "http://archive.ubuntu.com/ubuntu/pool/main/${dir}/")"
    file="$(grep -oE "${pattern}" <<<"${index}" | sort -uV | tail -1)"
    test -n "${file}"
    curl -s -f -o "${file}" "http://archive.ubuntu.com/ubuntu/pool/main/${dir}/${file}"
}
pool_fetch g/glibc 'libc6-dev_2\.39-0ubuntu8\.[0-9]+_amd64\.deb'
pool_fetch g/glibc 'libc6-dev-i386_2\.39-0ubuntu8\.[0-9]+_amd64\.deb'
pool_fetch g/glibc 'libc6-i386_2\.39-0ubuntu8\.[0-9]+_amd64\.deb'
pool_fetch l/linux 'linux-libc-dev_6\.8\.0-[0-9]+\.[0-9]+_amd64\.deb'

rm -rf "${SWD3_I386_SYSROOT}"
mkdir -p "${SWD3_I386_SYSROOT}"
for deb in *.deb; do
    dpkg-deb --fsys-tarfile "${deb}" | tar -x -m --no-same-owner -C "${SWD3_I386_SYSROOT}" 2>/dev/null || true
done
ln -sfn x86_64-linux-gnu/asm "${SWD3_I386_SYSROOT}/usr/include/asm"
test -f "${SWD3_I386_SYSROOT}/usr/lib32/libc.a"
test -f "${SWD3_I386_SYSROOT}/usr/lib/gcc/x86_64-linux-gnu/13/32/libstdc++.a"
ls "${debs}" > "${SWD3_TP}/i386-sysroot-packages.txt"
rm -rf "${debs}"
printf 'i386 sysroot ready at %s\n' "${SWD3_I386_SYSROOT}"
