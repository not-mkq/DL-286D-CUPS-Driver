{ lib
, stdenv
, fetchFromGitHub
, cups
}:

stdenv.mkDerivation rec {
  pname = "dl286d-cups-driver";
  version = "1.0.0";

  src = fetchFromGitHub {
    owner = "example";
    repo = "dl286d-cups-driver";
    rev = "v${version}";
    hash = lib.fakeSha256;
  };

  nativeBuildInputs = [ cups.dev ];
  buildInputs = [ cups ];

  buildPhase = ''
    gcc -O2 -std=c11 -Wall -Wextra $(cups-config --cflags --ldflags) \
      dl286d-raster.c -o dl286d-raster $(cups-config --libs raster)
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
    homepage = "https://github.com/example/dl286d-cups-driver";
    license = licenses.gpl3Plus;
    maintainers = [];
    platforms = platforms.linux;
  };
}
