# 得力 DL-286D CUPS 驱动

![得力 DL-286D](printer.jpg)

[English README](README.md)

## 概述
本仓库提供在 USB 抓包基础上逆向得到的 GPLv3 CUPS 滤镜 `dl286d-raster` 与简洁的 ppdc 描述 `dl286d.drv`，用于让得力 DL-286D 标签机在 Linux 上正常工作。滤镜读取 `application/vnd.cups-raster` 数据，完成缩放与抖动后输出设备所需的 ESC/POS 数据流。

> **限制**：目前仅支持有线 USB 连接，蓝牙模式尚未适配；标签尺寸也仅针对官方 40 mm × 50 mm 规格做过测试，其他尺寸可能出现偏移。

## 构建与安装
1. 编译滤镜并安装到 cupsd：
   ```bash
   gcc -O2 -std=c11 -Wall -Wextra \
     $(cups-config --cflags) $(cups-config --ldflags) \
     dl286d-raster.c -o dl286d-raster $(cups-config --image --libs)
   sudo install -m 755 dl286d-raster /usr/lib/cups/filter/
   ```
2. 生成并部署 PPD：
   ```bash
   mkdir -p ppd
   ppdc -d ppd dl286d.drv
   sudo install -m 644 ppd/dl286d.ppd /usr/share/cups/model/
   ```

## 测试
参考 `dl286d-raster.c` 中的示例：
```bash
cupsfilter -p /path/to/dl286d.ppd -m application/vnd.cups-raster input.png \
  | ./dl286d-raster \
  | lp -d dl286d-raw
```
准备 1 bpp 与 24/32 bpp 样例覆盖所有代码路径，确认抖动与阈值逻辑后再打印正式标签。

## 打包
仓库包含三套参考打包脚本：
- **Arch Linux**：修改 `PKGBUILD` 内的 `pkgname`/`source`，运行 `makepkg -si`。
- **Debian/Ubuntu**：更新 `debian/changelog` 的版本号，执行 `dpkg-buildpackage -us -uc`。
- **Nix/NixOS**：运行 `nix-build default.nix`，发布时将 `src`/`rev` 改为自己的标签。
本提交已在 `release/` 目录中生成对应的预编译包，可直接用于 GitHub Release 上传。

## 许可证与贡献
源码以 GPL-3.0-or-later 授权。驱动源自 USB 逆向推测，欢迎提供含复现步骤或 USB 抓包信息的 issue / PR，以便及时修复潜在问题。

**法律声明**：本项目完全依靠对该设备 USB 通讯数据的观察与分析完成，未使用厂家提供的任何源代码、二进制或受版权保护的资料；驱动属于独立的“洁净室”实现。
