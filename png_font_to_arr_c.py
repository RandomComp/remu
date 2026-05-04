from PIL import Image

img = Image.open("cp437-8x16.png").convert("RGB")

pixels = list(img.getdata())

width, height = img.size

c_width = 8

c_height = 16

font = [0 for i in range(width * height // 8)]

c_pos = 0

for i in range(width * height):
	avg_color = sum(pixels[i]) // 3

	index = i // 8
	bit_index = i % 8

	if (avg_color >= 128):
		font[index] |= 1 << (bit_index)

for pix in font:
	print(bin(pix))
