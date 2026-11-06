#!/usr/bin/env python3

from pathlib import Path
import subprocess
import sys


project_directory = Path(__file__).resolve().parent.parent
source_path = project_directory / "resources" / "icon.png"

# Haiku's 256-colour system palette. Entry 255 is the transparent CMAP8 value.
palette_hex = (
	"000000080808101010181818202020282828303030383838404040484848505050"
	"585858606060686868707070787878808080888888909090989898a0a0a0a8a8a8"
	"b0b0b0b8b8b8c0c0c0c8c8c8d0d0d0d8d8d8e0e0e0e8e8e8f0f0f0f8f8f8"
	"0000ff0000e50000cc0000b300009a00008100006900005000003700001eff0000"
	"e40000cb0000b200009900008000006900005000003700001e000000ff0000e400"
	"00cb0000b200009900008000006900005000003700001e00009833ffffffcbffffcb"
	"ffcbcbff98cbff66cbff33cbff0098ffff98ffcb98ff9898ff6698ff3398ff0066"
	"ffff66ffcb66ff9866ff6666ff3366ff0033ffff33ffcb33ff9833ff6633ff3333"
	"ff00ff98ffff98cbff9898ff9866ff9833ff98000066ff0066cbcbcbffcbcbcbcb"
	"cb98cbcb66cbcb33cbcb0098cbff98cbcb98cb9898cb6698cb3398cb0066cbff66"
	"cbcb66cb9866cb6666cb3366cb0033cbff33cbcb33cb9833cb6633cb3333cb00ff"
	"66ffff66cbff6698ff6666ff6633ff6600006698006666cb98ffcb98cbcb9898cb"
	"9866cb9833cb98009898ff9898cb9898989898669898339898006698ff6698cb66"
	"98986698666698336698003398ff3398cb339898339866339833339800e68600ff"
	"33cbff3398ff3366ff3333ff3300006633006600cb66ffcb66cbcb6698cb6666cb"
	"6633cb66009866ff9866cb9866989866669866339866006666ff6666cb66669866"
	"666666336666003366ff3366cb336698336666336633336600ff00ffff00cbff00"
	"98ff0066ff0033ffaf130033ff0033cbcb33ffcb33cbcb3398cb3366cb3333cb33"
	"009833ff9833cb9833989833669833339833006633ff6633cb6633986633666633"
	"336633003333ff3333cb333398333366333333333300ffcb66ffcb98ffcbcbffcb"
	"ff003398003366003333003300cb00ffcb00cbcb0098cb0066cb0033ffe3469800"
	"ff9800cb9800989800669800339800006600ff6600cb6600986600666600336600"
	"003300ff3300cb330098330066330033330000ffcb33ffcb00ffff00ffff33ffff66"
	"ffff98ffffcbffffff"
)

palette_bytes = bytes.fromhex(palette_hex)
palette = [tuple(palette_bytes[index:index + 3]) for index in range(0, 768, 3)]

if len(palette) != 256:
	print("error: invalid embedded Haiku palette", file=sys.stderr)
	sys.exit(1)

for size in (32, 16):
	command = [
		"ffmpeg", "-v", "error", "-i", str(source_path),
		"-vf", f"scale={size}:{size}:flags=lanczos",
		"-f", "rawvideo", "-pix_fmt", "rgba", "pipe:1"
	]
	pixels = subprocess.run(command, check=True, stdout=subprocess.PIPE).stdout
	if len(pixels) != size * size * 4:
		print(f"error: ffmpeg returned invalid {size} px image data",
			file=sys.stderr)
		sys.exit(1)

	indexed = bytearray()
	for offset in range(0, len(pixels), 4):
		red, green, blue, alpha = pixels[offset:offset + 4]
		if alpha < 128:
			indexed.append(255)
			continue

		best_index = min(range(255), key=lambda index:
			(2 * (red - palette[index][0]) ** 2
				+ 4 * (green - palette[index][1]) ** 2
				+ 3 * (blue - palette[index][2]) ** 2))
		indexed.append(best_index)

	output_path = project_directory / "resources" / f"icon-{size}.icon"
	output_path.write_bytes(indexed)
	print(f"wrote {output_path.relative_to(project_directory)}")
