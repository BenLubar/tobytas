package main

import (
	"archive/tar"
	"bufio"
	"compress/gzip"
	"image"
	"image/color"
	"image/draw"
	"io"
	"os"
	"strings"
)

func check(err error) {
	if err != nil {
		panic(err)
	}
}

func checkF(f func() error) {
	check(f())
}

const (
	key_z = 1 << iota
	key_x
	key_c
	key_up
	key_down
	key_left
	key_right
	key_a
	key_d
	key_i
	key_l
	key_n
	key_o
	key_r
	key_s
	key_u
	key_space
)

func main() {
	f, err := os.Open("tas.ltm")
	check(err)
	defer checkF(f.Close)
	zr, err := gzip.NewReader(f)
	check(err)
	defer checkF(zr.Close)
	tr := tar.NewReader(zr)

	for {
		h, err := tr.Next()
		check(err)
		if h.Name == "inputs" {
			bits := parse(tr)
			render(bits)
			break
		}
	}

	for {
		_, err := tr.Next()
		if err == io.EOF {
			break
		}
		check(err)
	}
}

func parse(r io.Reader) []uint32 {
	s := bufio.NewScanner(r)
	defer checkF(s.Err)

	var bits []uint32

	for s.Scan() {
		line := s.Text()
		line = line[1 : len(line)-1]
		if line == "" {
			bits = append(bits, 0)
			continue
		}

		active := strings.Split(line, ":")

		var b uint32

		for _, key := range active {
			switch key {
			case "20":
				b |= key_space
			case "61":
				b |= key_a
			case "63":
				b |= key_c
			case "64":
				b |= key_d
			case "69":
				b |= key_i
			case "6c":
				b |= key_l
			case "6e":
				b |= key_n
			case "6f":
				b |= key_o
			case "72":
				b |= key_r
			case "73":
				b |= key_s
			case "75":
				b |= key_u
			case "78":
				b |= key_x
			case "7a":
				b |= key_z
			case "ff51":
				b |= key_left
			case "ff52":
				b |= key_up
			case "ff53":
				b |= key_right
			case "ff54":
				b |= key_down
			default:
				panic(key)
			}
		}

		bits = append(bits, b)
	}

	return bits
}

// Remainder of this file is based on https://web.archive.org/web/20120619043838/http://code.google.com/p/brandon-evans-tas/source/browse/Lua/ddrinput.lua

var buttons = [...]string{
	`................
....xxxxxxxx....
...xOOOOOOOOx...
....xxxxxOOOx...
.........OOOx...
........xOOOx...
.......xOOOx....
......xOOOx.....
.....xOOOx......
....xOOOx.......
...xOOOx........
...xOOO.........
...xOOOxxxxx....
...xOOOOOOOOx...
....xxxxxxxx....
................`,
	`................
....xxx..xxx....
...xOOO..OOOx...
...xOOO..OOOx...
...xOOO..OOOx...
...xOOOxxOOOx...
....xOOOOOOx....
.....xOOOOx.....
.....xOOOOx.....
....xOOOOOOx....
...xOOOxxOOOx...
...xOOO..OOOx...
...xOOO..OOOx...
...xOOO..OOOx...
....xxx..xxx....
................`,
	`................
.....xxxxxx.....
....xOOOOOOx....
...xOOOxxOOOx...
...xOOO..OOOx...
...xOOO..OOOx...
...xOOO..xxx....
...xOOO.........
...xOOO.........
...xOOO..xxx....
...xOOO..OOOx...
...xOOO..OOOx...
...xOOOxxOOOx...
....xOOOOOOx....
.....xxxxxx.....
................`,
	`......xxx.......
.....xxOxx......
....xxOxOxx.....
...xxOxxxOxx....
..xxOxx.xxOxx...
.xxOxx...xxOxx..
xxOxxx...xxxOxx.
xOOOOx...xOOOOx.
xxxxOx...xOxxxx.
...xOx...xOx....
...xOx...xOx....
...xOx...xOx....
...xOx...xOx....
...xOxxxxxOx....
...xOOOOOOOx....
...xxxxxxxxx....`,
	`...xxxxxxxxx....
...xOOOOOOOx....
...xOxxxxxOx....
...xOx...xOx....
...xOx...xOx....
...xOx...xOx....
...xOx...xOx....
xxxxOx...xOxxxx.
xOOOOx...xOOOOx.
xxOxxx...xxxOxx.
.xxOxx...xxOxx..
..xxOxx.xxOxx...
...xxOxxxOxx....
....xxOxOxx.....
.....xxOxx......
......xxx.......`,
	`................
......xxx.......
.....xxOx.......
....xxOOx.......
...xxOxOxxxxxxxx
..xxOxxOOOOOOOOx
.xxOxxxxxxxxxxOx
xxOxx........xOx
xOxx.........xOx
xxOxx........xOx
.xxOxxxxxxxxxxOx
..xxOxxOOOOOOOOx
...xxOxOxxxxxxxx
....xxOOx.......
.....xxOx.......
......xxx.......`,
	`................
.......xxx......
.......xOxx.....
.......xOOxx....
xxxxxxxxOxOxx...
xOOOOOOOOxxOxx..
xOxxxxxxxxxxOxx.
xOx........xxOxx
xOx.........xxOx
xOx........xxOxx
xOxxxxxxxxxxOxx.
xOOOOOOOOxxOxx..
xxxxxxxxOxOxx...
.......xOOxx....
.......xOxx.....
.......xxx......`,
	`................
.....xxxxxx.....
.....OOOOOO.....
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
...xOOOOOOOOx...
...xOOOxxOOOx...
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
................`,
	`................
...xxxxxxxx.....
...xOOOOOOO.....
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
...xOOOxxOOxx...
...xOOOOOOO.....
................`,
	`................
....xxxxxxx.....
....xOOOOOx.....
.....xOOOx......
.....xOOOx......
.....xOOOx......
.....xOOOx......
.....xOOOx......
.....xOOOx......
.....xOOOx......
.....xOOOx......
.....xOOOx......
.....xOOOx......
....xxOOOxx.....
....xOOOOOx.....
................`,
	`................
...xxxx.........
...xOOx.........
...xOOx.........
...xOOx.........
...xOOx.........
...xOOx.........
...xOOx.........
...xOOx.........
...xOOx.........
...xOOx.........
...xOOx.........
...xOOx.........
...xOOOxxxxxx...
...xOOOOOOOOx...
................`,
	`................
...xxxx..xxxx...
...xOOx..xOOx...
...xOOx..xOOx...
...xOOOx.xOOx...
...xOOOO.xOOx...
...xOOOOOOOOx...
...xOOOxOOOOx...
...xOOx.OOOOx...
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
................`,
	`................
.....xxxxxx.....
.....OOOOOO.....
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
...xxOOxxOOxx...
.....OOOOOO.....
................`,
	`................
...xxxxxxxx.....
...xOOOOOOO.....
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
...xOOOxxOOxx...
...xOOOOOOO.....
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
................`,
	`................
.....xxxxxx.....
.....OOOOOO.....
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
.....OOO........
.....xOOxx......
......xOOx......
........OOO.....
...xxxx.xOOxx...
...xOOx..xOOx...
...xOOx..xOOx...
...xxOOxxOOxx...
.....OOOOOO.....
................`,
	`................
...xxxx..xxxx...
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
...xOOx..xOOx...
...xxOOxxOOxx...
.....OOOOOO.....
................`,
	`................
................
................
................
................
................
................
................
................
...xxx....xxx...
..xOOOxxxxOOOx..
..xOxOOOOOOxOx..
..xOxxxxxxxxOx..
..xOOOOOOOOOOx..
...xxxxxxxxxx...
................`,
}

var buttonMasks = [17][2]*image.Alpha{
	makeMask(buttons[0]),
	makeMask(buttons[1]),
	makeMask(buttons[2]),
	makeMask(buttons[3]),
	makeMask(buttons[4]),
	makeMask(buttons[5]),
	makeMask(buttons[6]),
	makeMask(buttons[7]),
	makeMask(buttons[8]),
	makeMask(buttons[9]),
	makeMask(buttons[10]),
	makeMask(buttons[11]),
	makeMask(buttons[12]),
	makeMask(buttons[13]),
	makeMask(buttons[14]),
	makeMask(buttons[15]),
	makeMask(buttons[16]),
}

func makeMask(str string) [2]*image.Alpha {
	oimg := image.NewAlpha(image.Rect(0, 0, buttonSize, buttonSize))
	ximg := image.NewAlpha(image.Rect(0, 0, buttonSize, buttonSize))

	i, j := 0, 0
	for y := 0; y < buttonSize; y++ {
		for x := 0; x < buttonSize; x++ {
			switch str[i] {
			case 'O':
				oimg.Pix[j] = 255
			case 'x':
				ximg.Pix[j] = 255
			}

			i++
			j++
		}
		i++
	}

	return [2]*image.Alpha{oimg, ximg}
}

const (
	buttonCount      = 7
	extraButtonCount = len(buttons) - buttonCount
	width, height    = 640, 120

	// button size in pixels (square)
	buttonSize = 16

	// pixels per frame moved by inputs.
	step = 3

	// time in frames to go through the rainbow; must be divisible by 6
	period = 600

	// glow time in frames
	glowHold = 10

	// buttons are pressed when they reach this x position
	target = (width - buttonSize) / 2

	// top of first button
	top = (height - buttonCount*buttonSize) / 2

	// number of inputs on the screen (in both directions)
	display = (target + buttonSize + step) / step
)

func render(bits []uint32) {
	img := image.NewRGBA(image.Rect(0, 0, width, height))
	left := target
	for _, b := range bits {
		left += step
		if b != 0 {
			break
		}
	}

	for trueFrame := range bits {
		left -= step
		if left < 0 {
			left = 0
		}
		draw.Draw(img, image.Rect(left, 0, width, height), image.Black, image.ZP, draw.Src)

		for btn := 0; btn < buttonCount+extraButtonCount; btn++ {
			var (
				// detect holds by going forward, then draw the symbols backward
				backString [display*2 + 1]bool
				glowLevel  [display*2 + 1]int
				any        bool
			)

			y := top + buttonSize*btn
			if btn >= buttonCount {
				y = (height - buttonSize) * (btn - buttonCount) / (extraButtonCount - 1)
			}

			for i := -display; i <= display; i++ {
				offset := display + i
				refFrame := trueFrame + i
				if refFrame < 0 || refFrame >= len(bits) {
					backString[offset] = false
					glowLevel[offset] = 0
				} else {
					if bits[refFrame]&(1<<uint(btn)) == 0 {
						backString[offset] = false
						if i == -display || glowLevel[offset-1] <= 0 {
							glowLevel[offset] = 0
						} else {
							glowLevel[offset] = glowLevel[offset-1] - 1
						}
					} else {
						any = true
						backString[offset] = true
						if i == -display {
							glowLevel[offset] = 1
						} else {

							glowLevel[offset] = glowHold
						}
					}
				}
			}

			for i := display; i >= 0; i-- {
				offset := display + i
				if backString[offset] {
					fg := retrieveColor(trueFrame + i)
					if backString[offset-1] {
						for k := 0; k < step; k++ {
							overlay(img, buttonMasks[btn], image.Pt(target+i*step-k, y), fg, color.RGBA{})
						}
					} else {
						overlay(img, buttonMasks[btn], image.Pt(target+i*step, y), fg, color.RGBA{0, 0, 0, 255})
					}
				}
			}
			for i := -1; i >= -display; i-- {
				offset := display + i
				if backString[offset] {
					fg := retrieveColor(trueFrame + i)
					fg.R = fg.R/8 + 16
					fg.G = fg.G/8 + 16
					fg.B = fg.B/8 + 16
					if offset > 0 && backString[offset-1] {
						for k := 0; k < step; k++ {
							overlay(img, buttonMasks[btn], image.Pt(target+i*step-k, y), fg, color.RGBA{})
						}
					} else {
						overlay(img, buttonMasks[btn], image.Pt(target+i*step, y), fg, color.RGBA{0, 0, 0, 255})
					}
				}
			}

			if any || btn < buttonCount {
				mh := uint8(255 * glowLevel[display] / glowHold)
				overlay(img, buttonMasks[btn], image.Pt(target-1, y), color.RGBA{192, 192, 192, 255}, color.RGBA{mh, mh, mh, 255})
			}
		}

		_, err := os.Stdout.Write(img.Pix)
		check(err)
	}
}

func overlay(img *image.RGBA, mask [2]*image.Alpha, pt image.Point, col, xcol color.RGBA) {
	rect := image.Rect(0, 0, buttonSize, buttonSize).Add(pt)
	draw.DrawMask(img, rect, image.NewUniform(col), image.ZP, mask[0], image.ZP, draw.Over)
	draw.DrawMask(img, rect, image.NewUniform(xcol), image.ZP, mask[1], image.ZP, draw.Over)
}

func retrieveColor(refFrame int) color.RGBA {
	phase := refFrame % period
	if phase < 0 {
		phase += period
	}
	sixth := period / 6
	i := phase / sixth
	part := phase % sixth
	frac := uint8(255 * part / (sixth - 1))

	c := color.RGBA{A: 255}
	switch i {
	case 0:
		c.R = 255
		c.G = frac
	case 1:
		c.R = 255 - frac
		c.G = 255
	case 2:
		c.G = 255
		c.B = frac
	case 3:
		c.G = 255 - frac
		c.B = 255
	case 4:
		c.R = frac
		c.B = 255
	case 5:
		c.R = 255
		c.B = 255 - frac
	}
	return c
}
