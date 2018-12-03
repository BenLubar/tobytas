#!/bin/bash -e

ffmpeg -i undertale.mp4 -i deltarune.mp4 -f rawvideo -pix_fmt rgba -s 640x360 -r 30 -i <(go run readout.go) -filter_complex 'nullsrc=size=2560x1440:rate=30 [base]; [0:v] scale=1280:960:flags=neighbor [left]; [base][left] overlay=shortest=1 [tmp1]; [tmp1][1:v] overlay=shortest=1:x=1280 [tmp2]; [2:v] scale=2560:1440:flags=neighbor [bottom]; [tmp2][bottom] overlay=shortest=1 [vout]; [0:a][1:a] amerge=inputs=2,pan=stereo|c0<c0+c1+c2|c1<c1+c2+c3 [aout]' -map '[vout]' -map '[aout]' -crf 0 -tune animation -preset veryslow -movflags +faststart -y -r 30 tas.mp4
