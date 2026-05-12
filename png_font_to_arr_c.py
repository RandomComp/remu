from PIL import Image, ImageDraw

from pyperclip import copy

img = Image.open("cp437-8x16.png").convert("RGB")

draw = ImageDraw.Draw(img)

pixels = list(img.getdata())

width, height = img.size

c_width = 8

c_height = 16

result = ""

cur_c = 0

for i in range(0, height, c_height):
	for j in range(0, width, c_width):
		for y in range(c_height):
			result += "0b"

			for x in range(c_width):
				pos = j + x + ((i + y) * width)

				avg_color = sum(pixels[pos]) / len(pixels[pos])

				if (avg_color >= 128):
					result += "1"
				else:
					result += "0"

			result += ",\n"

		draw.point((j, i), (0, 255, 0))

		cur_c += 1

result += "\n"

print(result)

copy(result)

img.show()
