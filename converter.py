#!/usr/bin/env python3

from PIL import Image
import argparse
from pathlib import Path


def convert(input_png, output_header, frame_ms=100):
    # Load image and convert to RGBA so transparency is handled.
    img = Image.open(input_png).convert("RGBA")

    FRAME_W = 5
    FRAME_H = 5

    # Expect a horizontal strip of 5x5 frames.
    if img.height != FRAME_H:
        raise ValueError(
            f"Image height must be {FRAME_H}px, "
            f"but got {img.height}px."
        )

    if img.width % FRAME_W != 0:
        raise ValueError(
            f"Image width must be a multiple of {FRAME_W}px, "
            f"but got {img.width}px."
        )

    num_frames = img.width // FRAME_W
    frames = []

    for f in range(num_frames):
        pixels = []

        # Row-major LED order:
        # index = x + 5*y
        # LED 0 = top-left, LED 5 = leftmost pixel of row 1.
        for y in range(FRAME_H):
            for x in range(FRAME_W):
                px = img.getpixel((
                    f * FRAME_W + x,
                    y
                ))

                r, g, b, a = px

                # Fully transparent pixels become black.
                # Partially transparent pixels are composited
                # against black.
                if a < 255:
                    r = (r * a + 127) // 255
                    g = (g * a + 127) // 255
                    b = (b * a + 127) // 255

                pixels.append((r, g, b))

        frames.append(pixels)

    # Generate C++ header.
    lines = [
        "#pragma once",
        "#include <stdint.h>",
        "",
        f"constexpr uint16_t NUM_FRAMES = {num_frames};",
        "constexpr uint8_t NUM_PIXELS = 25;",
        f"constexpr uint16_t FRAME_DURATION_MS = {frame_ms};",
        "",
        "// Format: animation[frame][pixel][RGB]",
        "// Pixel index = x + 5*y",
        f"const uint8_t animation[{num_frames}][25][3] = {{",
    ]

    for frame_index, pixels in enumerate(frames):
        lines.append(f"    {{ // Frame {frame_index}")

        for y in range(FRAME_H):
            row = pixels[y * FRAME_W:(y + 1) * FRAME_W]

            pixel_data = ", ".join(
                f"{{{r}, {g}, {b}}}"
                for r, g, b in row
            )

            comma = "," if y < FRAME_H - 1 else ""
            lines.append(f"        {pixel_data}{comma}")

        comma = "," if frame_index < num_frames - 1 else ""
        lines.append(f"    }}{comma}")

    lines.append("};")
    lines.append("")

    Path(output_header).write_text(
        "\n".join(lines),
        encoding="utf-8"
    )

    print(f"Converted {num_frames} frames.")
    print(f"Frame size: {FRAME_W}x{FRAME_H}")
    print(f"Frame duration: {frame_ms} ms")
    print(f"Output: {output_header}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Convert a 5x5 Aseprite sprite sheet to C++."
    )

    parser.add_argument("input", help="Input horizontal PNG sprite sheet")
    parser.add_argument(
        "-o", "--output",
        default="animation.h",
        help="Output header (default: animation.h)"
    )
    parser.add_argument(
        "--frame-ms",
        type=int,
        default=100,
        help="Duration of each frame in milliseconds (default: 100)"
    )

    args = parser.parse_args()

    if args.frame_ms <= 0:
        parser.error("--frame-ms must be greater than zero")

    convert(args.input, args.output, args.frame_ms)