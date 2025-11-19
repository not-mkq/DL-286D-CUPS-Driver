pkgname=dl286d-cups-driver
pkgver=1.0.0
pkgrel=1
pkgdesc="CUPS raster filter and PPD for the Deli DL-286D label printer"
arch=('x86_64' 'aarch64')
url="https://github.com/example/dl286d-cups-driver"
license=('GPL3')
depends=('cups')
makedepends=('cups' 'gcc')
source=("$pkgname-$pkgver.tar.gz::https://github.com/example/dl286d-cups-driver/archive/refs/tags/v$pkgver.tar.gz")
sha256sums=('SKIP')

build() {
  cd "$srcdir/${pkgname}-${pkgver}"

  gcc -O2 -std=c11 -Wall -Wextra $(cups-config --cflags --ldflags) \
    dl286d-raster.c -o dl286d-raster $(cups-config --libs raster)

  mkdir -p ppd
  ppdc -d ppd dl286d.drv
}

package() {
  cd "$srcdir/${pkgname}-${pkgver}"

  install -Dm755 dl286d-raster "$pkgdir/usr/lib/cups/filter/dl286d-raster"
  install -Dm644 ppd/dl286d.ppd "$pkgdir/usr/share/cups/model/dl286d.ppd"
  install -Dm644 README.md "$pkgdir/usr/share/doc/${pkgname}/README.md"
  install -Dm644 LICENSE "$pkgdir/usr/share/licenses/${pkgname}/LICENSE"
}
