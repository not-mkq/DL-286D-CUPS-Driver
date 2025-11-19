# Repository Guidelines

## Project Structure & Module Organization
The repository is intentionally small: `dl286d-raster.c` contains the entire CUPS raster filter that ingests application data and emits ESC/POS segments, while `dl286d.drv` is the ppdc source that produces the printable PPD (`dl286d.ppd`). Keep experimental scripts in ad‑hoc branches; only shipping assets should live next to the filter so packaging remains trivial.

## Build, Test, and Development Commands
- `gcc -O2 -std=c11 -Wall -Wextra $(cups-config --cflags --ldflags) dl286d-raster.c -o dl286d-raster $(cups-config --libs raster)` – builds the standalone raster binary against the system CUPS headers/libraries.
- `ppdc dl286d.drv` – emits `dl286d.ppd`; copy it into `/usr/share/ppd` (or your distro override) so CUPS can surface the model.
- `cupsfilter -p path/to/dl286d.ppd -m application/vnd.cups-raster sample.png | ./dl286d-raster > out.bin` – dry-run pipeline that verifies the filter without touching a printer; inspect `out.bin` with a hex viewer or feed it to a raw queue via `lp -d dl286d-raw out.bin`.

## Coding Style & Naming Conventions
Source is plain C11 with two-space indentation, K&R braces, and 100-character soft wrap. Prefer `static const` tables (see the Bayer matrix) over heap allocations, and favor descriptive snake_case names (`use_dither`, `WIDTH_DOTS`). Guard log noise by routing diagnostics through `fprintf(stderr, …)`. When touching protocol bytes, annotate intent with short `//` comments so byte meanings stay obvious for reverse-engineering.

## Testing Guidelines
There is no automated test harness; rely on deterministic fixtures. Maintain a `fixtures/` folder locally with representative PNG/PDF inputs, then script `/usr/bin/cupsfilter` + `dl286d-raster` + `lp -d dl286d-raw` to confirm scaling, dithering, and suffix padding. Validate edge cases: tiny widths, >320 px heights, and both 1 bpp and 24/32 bpp inputs. When altering ESC/POS framing, capture USB traffic (e.g., `usbmon`) to compare against the previous release before shipping.

## Commit & Pull Request Guidelines
Even without a published history, follow imperative, component-scoped messages such as `filter: clamp dithering thresholds` or `drv: expose Label media`. Each pull request must state the problem, summarize the solution, and include validation evidence (command transcripts or printer photos). Link related tickets and note any deployment steps (e.g., “reinstall PPD and restart cupsd”). If UI-visible output changes, attach before/after label scans.
