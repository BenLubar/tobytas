#!/bin/sh -e

ffmpeg -i undertale.mp4 -i deltarune.mp4 -filter_complex 'nullsrc=size=2560x960 [base]; [0:v] scale=1280:960:flags=neighbor [left]; [base][left] overlay=shortest=1 [tmp1]; [tmp1][1:v] overlay=shortest=1:x=1280 [vout]; [0:a][1:a] amerge=inputs=2,pan=stereo|c0<c0+c1|c1<c2+c3 [aout]' -map '[vout]' -map '[aout]' -crf 0 -movflags +faststart -y tas.mp4
