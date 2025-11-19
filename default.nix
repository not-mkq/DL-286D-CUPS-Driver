{ lib
, stdenv
, cups
, src ? lib.cleanSource ./. 
}:

stdenv.mkDerivation rec {
  pname = "dl286d-cups-driver";
  version = "1.0";

  inherit src;

  nativeBuildInputs = [ cups.dev ];
  buildInputs = [ cups ];

  buildPhase = ''
    cups_cflags=$(cups-config --cflags)
    cups_ldflags=$(cups-config --ldflags)
    cups_libs=$(cups-config --image --libs)
    gcc -O2 -std=c11 -Wall -Wextra $cups_cflags $cups_ldflags \
      dl286d-raster.c -o dl286d-raster $cups_libs
    mkdir -p ppd
    ppdc -d ppd dl286d.drv
  '';

  installPhase = ''
    runHook preInstall
    install -Dm755 dl286d-raster $out/lib/cups/filter/dl286d-raster
    install -Dm644 ppd/dl286d.ppd $out/share/cups/model/dl286d.ppd
    install -Dm644 README.md $out/share/doc/${pname}/README.md
    install -Dm644 LICENSE $out/share/doc/${pname}/LICENSE
    runHook postInstall
  '';

  meta = with lib; {
    description = "CUPS raster filter and PPD for the Deli DL-286D label printer";
    homepage = "https://github.com/not-mkq/DL-286D-CUPS-Driver";
    license = licenses.gpl3Plus;
    maintainers = [];
    platforms = platforms.linux;
  };
}
