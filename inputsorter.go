package main

import (
	"os"
	"sort"
	"strconv"
	"strings"
)

// inputOrder is Xlib_default_keymap from the following file in libTAS,
// converted to hexadecimal strings, with NoSymbol entries removed.
// https://github.com/clementgallet/libTAS/blob/master/src/library/inputs/xkeyboardlayout.cpp
var inputOrder = [...]string{
	"ff1b", "31", "32", "33", "34", "35", "36", "37", "38", "39", "30",
	"2d", "3d", "ff08", "ff09", "71", "77", "65", "72", "74", "79", "75",
	"69", "6f", "70", "5b", "5d", "ff0d", "ffe3", "61", "73", "64", "66",
	"67", "68", "6a", "6b", "6c", "3b", "27", "60", "ffe1", "5c", "7a",
	"78", "63", "76", "62", "6e", "6d", "2c", "2e", "2f", "ffe2", "ffaa",
	"ffe9", "20", "ffe5", "ffbe", "ffbf", "ffc0", "ffc1", "ffc2", "ffc3",
	"ffc4", "ffc5", "ffc6", "ffc7", "ff7f", "ff14", "ff95", "ff97",
	"ff9a", "ffad", "ff96", "ff9d", "ff98", "ffab", "ff9c", "ff99",
	"ff9b", "ff9e", "ff9f", "fe03", "3c", "ffc8", "ffc9", "ff26", "ff25",
	"ff23", "ff27", "ff22", "ff8d", "ffe4", "ffaf", "ff61", "ffea",
	"ff0a", "ff50", "ff52", "ff55", "ff51", "ff53", "ff57", "ff54",
	"ff56", "ff63", "ffff", "1008ff12", "1008ff11", "1008ff13",
	"1008ff2a", "ffbd", "b1", "ff13", "1008ff4a", "ffae", "ff31", "ff34",
	"ffeb", "ffec", "ff67", "ff69", "ff66", "ff65", "1008ff57",
	"1008ff6b", "1008ff6d", "ff68", "1008ff58", "ff6a", "1008ff65",
	"1008ff1d", "1008ff2f", "1008ff2b", "1008ff5d", "1008ff7b",
	"1008ff8a", "1008ff41", "1008ff42", "1008ff2e", "1008ff5a",
	"1008ff2d", "1008ff74", "1008ff7f", "1008ff19", "1008ff30",
	"1008ff33", "1008ff26", "1008ff27", "1008ff2c", "1008ff2c",
	"1008ff17", "1008ff14", "1008ff16", "1008ff15", "1008ff1c",
	"1008ff3e", "1008ff6e", "1008ff81", "1008ff18", "1008ff73",
	"1008ff56", "1008ff78", "1008ff79", "28", "29", "1008ff68", "ff66",
	"1008ff81", "1008ff45", "1008ff46", "1008ff47", "1008ff48",
	"1008ff49", "1008ffb2", "1008ffa9", "1008ffb0", "1008ffb1", "ff7e",
	"1008ff14", "1008ff31", "1008ff43", "1008ff44", "1008ff4b",
	"1008ffa7", "1008ff56", "1008ff14", "1008ff97", "ff61", "1008ff8f",
	"1008ff19", "1008ff8e", "1008ff1b", "1008ff5f", "1008ff3c",
	"1008ff5e", "1008ff36", "ff69", "1008ff03", "1008ff02", "1008ff32",
	"1008ff59", "1008ff04", "1008ff06", "1008ff05", "1008ff7b",
	"1008ff72", "1008ff90", "1008ff77", "1008ff5b", "1008ff93",
	"1008ff94", "1008ff95",
}

var orderMap = func() map[string]int {
	m := make(map[string]int)

	for i, s := range inputOrder {
		m[s] = i + 1
	}

	return m
}()

func main() {
	b, err := os.ReadFile("tas/inputs")
	if err != nil {
		panic(err)
	}

	lines := strings.SplitAfter(string(b), "\n")

	for i := range lines {
		if i == len(lines)-1 && lines[i] == "" {
			continue
		}

		lines[i] = sortLine(lines[i])
	}

	err = os.WriteFile("tas/inputs", []byte(strings.Join(lines, "")), 0664)
	if err != nil {
		panic(err)
	}
}

func sortLine(line string) string {
	if !strings.HasPrefix(line, "|K") || !strings.HasSuffix(line, "|\n") {
		panic("unexpected input recording line format: " + strconv.Quote(line))
	}

	keys := strings.Split(line[2:len(line)-2], ":")

	sort.Sort(sortKeys(keys))

	return "|K" + strings.Join(keys, ":") + "|\n"
}

type sortKeys []string

func (s sortKeys) Len() int           { return len(s) }
func (s sortKeys) Swap(i, j int)      { s[i], s[j] = s[j], s[i] }
func (s sortKeys) Less(i, j int) bool { return orderMap[s[i]] < orderMap[s[j]] }
