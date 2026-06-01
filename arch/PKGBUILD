# Maintainer: proximacentav
pkgname=maxfetch
pkgver=1.0
pkgrel=1
pkgdesc="system information programm"
arch=('x86_64' 'aarch64')
url="https://github.com/proximacentav/maxfetch"
license=('GPLv3')
depends=('gcc-libs' 'glibc')
makedepends=('gcc' 'make' 'base-devel')
source=("maxfetch.cpp")
sha256sums=('C1a20420ad98b8257fe74bef9a55bcfea792fe5a1f1006b9b0f9a80a73f8a9af7')

build() {
    g++ -std=c++17 -O2 -o maxfetch maxfetch.cpp
}

package() {
    install -Dm755 maxfetch "$pkgdir/usr/bin/maxfetch"
}
#sha256sums=('C1a20420ad98b8257fe74bef9a55bcfea792fe5a1f1006b9b0f9a80a73f8a9af7')
