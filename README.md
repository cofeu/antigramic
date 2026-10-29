# AntigramicOS

AntigramicOS is an experimental x86_64 operating system. Its current boot target
is a portrait, touch-friendly launcher with Turkish text and small placeholder
apps.

## Build and run

```sh
make
qemu-system-x86_64 -m 128M -vga std -rtc base=localtime \
  -drive format=raw,file=build/x86_64/bootloader_bios.bin -serial stdio
```

Use the mouse or keyboard to open app tiles. On the keyboard, `1`–`6` opens an
app, left and right arrows move the selection, `Enter` opens it, and `Esc` or
`H` returns home. The phone dial pad accepts mouse and number-key input.
Messages can be typed and sent locally; the browser address can be edited.
Settings switches and brightness control respond to clicks, and Files lists the
assets embedded in the kernel.

The launcher supports up to four pages. Tap the `+` control below the dock to
add a page, use the page dots or left/right keys to switch pages, and drag an
app icon horizontally to move it to another page. Settings cycles through the
bundled night, ocean, and sunset wallpaper color styles.

App launch uses an icon-origin zoom animation; returning home is instant by
default. In Settings, “Ana ekran animasyonu” enables the matching return zoom.
“Hareket hızı” switches animated transitions between 120 FPS, 60 FPS, and
reduced motion. “Animasyon süresi” controls the duration of app, home, and
launcher-page transitions. QEMU and the host display can present fewer frames
than the guest target, depending on their refresh rate and rendering speed.
Settings also offers 480x854, 544x960, and 640x1136 VBE modes. Developer mode
shows the active display mode in the status bar.

The UI uses 1080x1920 design coordinates, renders the bundled `assets/images/home.png`
wallpaper with the kernel PNG decoder, and reads the clock and date from the CMOS
real-time clock. DejaVu Sans bitmap glyphs include Turkish characters. QEMU 8.2's standard VGA adapter currently displays it at
544x960 in a portrait window; the VBE adapter rounds widths to multiples of
eight and limits the supported mode dimensions. `make font-data` regenerates
the baked glyph tables from `assets/fonts/DejaVuSans.ttf` (requires Pillow).

The phone, messages, browser, settings, files, and music screens currently provide
local UI interactions. Cellular, network, filesystem, and audio device drivers
are not implemented yet, so calls, Internet access, persistent messages/files,
and sound playback are unavailable.
