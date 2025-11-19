# Deli DL-286D CUPS Driver / 得力 DL-286D CUPS 驱动

## Overview / 概述
English: This repository hosts a GPLv3 CUPS raster filter (`dl286d-raster`) and a minimal ppdc source (`dl286d.drv`) that were reverse-engineered from USB traces of the Deli DL-286D label printer. The filter ingests `application/vnd.cups-raster` data, applies scaling plus ordered dithering, and emits the ESC/POS sequence expected by the printer.
中文：本仓库提供在 USB 抓包基础上逆向得到的 GPLv3 CUPS 滤镜 `dl286d-raster` 与简洁的 ppdc 描述 `dl286d.drv`，用于让得力 DL-286D 标签机在 Linux 上正常工作。滤镜读取 `application/vnd.cups-raster` 数据，完成缩放与抖动后输出设备所需的 ESC/POS 数据流。

## Build & Install / 构建与安装
English:
1. Build the raster filter and install it for cupsd:
   ```bash
   gcc -O2 -std=c11 -Wall -Wextra $(cups-config --cflags --ldflags) dl286d-raster.c -o dl286d-raster $(cups-config --libs raster)
   sudo install -m 755 dl286d-raster /usr/lib/cups/filter/
   ```
2. Compile the PPD and install it:
   ```bash
   ppdc dl286d.drv
   sudo install -m 644 ppd/dl286d.ppd /usr/share/cups/model/
   ```
中文：
1. 编译滤镜并安装到 cupsd：
   ```bash
   gcc -O2 -std=c11 -Wall -Wextra $(cups-config --cflags --ldflags) dl286d-raster.c -o dl286d-raster $(cups-config --libs raster)
   sudo install -m 755 dl286d-raster /usr/lib/cups/filter/
   ```
2. 生成并部署 PPD：
   ```bash
   ppdc dl286d.drv
   sudo install -m 644 ppd/dl286d.ppd /usr/share/cups/model/
   ```

## Testing / 测试
English: Follow the pipeline embedded in `dl286d-raster.c`—run `cupsfilter -p /path/to/dl286d.ppd -m application/vnd.cups-raster input.png | ./dl286d-raster | lp -d dl286d-raw` to validate scaling and dithering through a raw queue. Keep fixtures for 1 bpp and 24/32 bpp images to confirm both code paths.
中文：按照 `dl286d-raster.c` 中的示例执行 `cupsfilter -p /path/to/dl286d.ppd -m application/vnd.cups-raster input.png | ./dl286d-raster | lp -d dl286d-raw`，通过 raw 队列验证缩放与抖动效果，并准备 1 bpp 与 24/32 bpp 样例覆盖所有代码路径。

## Packaging / 打包
English: Three reference packages live in this repo.
- Arch Linux: edit `pkgname`/`source` in `PKGBUILD`, then run `makepkg -si`.
- Debian/Ubuntu: bump `Version` in `debian/changelog`, then use `dpkg-buildpackage -us -uc`.
- Nix/NixOS: `nix-build` will compile and stage the driver using `default.nix`; override `src` with your Git tag when publishing.
中文：仓库包含三套打包脚本。
- Arch Linux：根据需要修改 `PKGBUILD` 中的 `pkgname`/`source`，执行 `makepkg -si` 安装。
- Debian/Ubuntu：更新 `debian/changelog` 的版本号后运行 `dpkg-buildpackage -us -uc`。
- Nix/NixOS：使用 `default.nix` 运行 `nix-build`，对外发布时将 `src` 指向自己的 Git 标签。
English note: the packaging files currently point to `https://github.com/example/dl286d-cups-driver`; replace that placeholder with your actual GitHub repository URL and tag before uploading to any archive.
中文提示：打包脚本里的 `https://github.com/example/dl286d-cups-driver` 仅为占位符，上线前请替换为自己真实的 GitHub 仓库地址与标签。

## License & Contributions / 许可与贡献
English: Licensed under GPL-3.0-or-later as noted in the source banner. Bug reports and pull requests that include reproduction steps or USB traces are welcome; please call out any deviations from real hardware since this code is based on best-effort reverse engineering.
中文：源码以 GPL-3.0-or-later 授权。驱动源自 USB 逆向推测，欢迎提供含复现步骤或 USB 抓包信息的 issue / PR，方便一同改进与修复潜在问题。
English (Legal notice): This project was developed by observing USB communication of the device. No source code, binaries, or copyrighted material from the manufacturer were used. The driver is an independent clean-room implementation.
中文（法律声明）：本项目完全依靠对该设备 USB 通讯数据的观察与分析完成，未使用厂家提供的任何源代码、二进制或受版权保护的资料；驱动属于独立的“洁净室”实现。
