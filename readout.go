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

func parse(r io.Reader) []uint8 {
	s := bufio.NewScanner(r)
	defer checkF(s.Err)

	var bits []uint8

	for s.Scan() {
		line := s.Text()
		line = line[1 : len(line)-1]
		if line == "" {
			bits = append(bits, 0)
			continue
		}

		active := strings.Split(line, ":")

		var b uint8

		for _, key := range active {
			switch key {
			case "63":
				b |= key_c
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

var buttons = [7]string{
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
}

const (
	delay   = 0
	display = 300

	period = 600 // for color; make it a multiple of 6

	merror = 10
)

func render(bits []uint8) {
	var (
		// detect holds by going forward, then draw the symbols backward
		// this array is necessary
		backstring [7][merror + display + 1]bool
		glowlevel  [7][merror + display + 1]int
	)

	img := image.NewRGBA(image.Rect(0, 0, 640, 120))
	for absframe := range bits {
		draw.Draw(img, img.Rect, image.Black, image.ZP, draw.Src)
		trueframe := absframe - delay
		for btn := 0; btn < 7; btn++ {
			for i := -merror; i <= display; i++ {
				merof := merror + i
				refframe := trueframe + i
				if refframe < 0 || refframe >= len(bits) {
					backstring[btn][merof] = false
					glowlevel[btn][merof] = 0
				} else {
					if bits[refframe]&(1<<uint(btn)) == 0 {
						backstring[btn][merof] = false
						if i == -merror {
							glowlevel[btn][merof] = 0
						} else {
							glowlevel[btn][merof] = glowlevel[btn][merof-1] - 1
						}
					} else {
						backstring[btn][merof] = true
						if i == -merror {
							glowlevel[btn][merof] = 1
						} else {

							glowlevel[btn][merof] = 10
						}
					}
				}
			}
			for i := display; i >= 0; i-- {
				merof := merror + i
				refframe := trueframe + i
				if refframe >= len(bits) {
					continue
				}
				if backstring[btn][merof] {
					if backstring[btn][merof-1] {
						for k := 0; k < 3; k++ {
							soverlay(img, buttons[btn], image.Pt(10+i*3-k, 4+16*btn), retrievecolor(refframe), color.RGBA{})
						}
					} else {
						soverlay(img, buttons[btn], image.Pt(10+i*3, 4+16*btn), retrievecolor(refframe), color.RGBA{0, 0, 0, 255})
					}
				}
			}

			mh := uint8(255 * glowlevel[btn][merror] / 10)
			soverlay(img, buttons[btn], image.Pt(10-1, 4+16*btn), color.RGBA{192, 192, 192, 255}, color.RGBA{mh, mh, mh, 255})
		}
		_, err := os.Stdout.Write(img.Pix)
		check(err)
	}
}

func soverlay(img *image.RGBA, str string, pt image.Point, col, ocol color.RGBA) {
	for x := 0; x < 16; x++ {
		for y := 0; y < 16; y++ {
			chr := str[x+y*17]
			if chr == 'O' && col.A != 0 {
				img.SetRGBA(pt.X+x, pt.Y+y, col)
			} else if chr == 'x' && ocol.A != 0 {
				img.SetRGBA(pt.X+x, pt.Y+y, ocol)
			}
		}
	}
}

func retrievecolor(refframe int) color.RGBA {
	phase := refframe % period
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
