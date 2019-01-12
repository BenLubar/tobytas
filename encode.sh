#!/bin/bash -e

source <(grep '^\(\(rerecord\|frame\)_count\|framerate_\(num\|den\)\)=[0-9]\+$' tas/config.ini)

csec=$(( $frame_count * $framerate_den * 100 / $framerate_num ))
sec=$(( $csec / 100 ))
min=$(( $sec / 60 ))
hrs=$(( $min / 60 ))
csec=$(( $csec % 100 ))
sec=$(( $sec % 60 ))
min=$(( $min % 60 ))

ffmpeg -lavfi $'color=size=2560x480,drawtext=fontfile=DTM-Sans.otf:fontsize=133.3:fontcolor=white:x=w/4-tw/2:y=h/2-lh*2:text=\'This is a\',drawtext=fontfile=DTM-Sans.otf:fontsize=133.3:fontcolor=white:x=w/4-tw/2:y=h/2-lh/2:text=\'Tool-Assisted\',drawtext=fontfile=DTM-Sans.otf:fontsize=133.3:fontcolor=white:x=w/4-tw/2:y=h/2+lh:text=\'Speedrun.\',drawtext=fontfile=DTM-Sans.otf:fontsize=100:fontcolor=white:x=w*3/4-tw/2:y=h/2-lh*2:text=\'For more\',drawtext=fontfile=DTM-Sans.otf:fontsize=100:fontcolor=white:x=w*3/4-tw/2:y=h/2-lh/2:text=\'information, visit\',drawtext=fontfile=DTM-Sans.otf:fontsize=100:fontcolor=white:x=w*3/4-tw/2:y=h/2+lh:text=\'http\\://tasvideos.org\',format=pix_fmts=monob,scale=size=640x120:flags=neighbor' -frames 1 -y tas-splash-1.png
ffmpeg -lavfi 'color=size=2560x480,drawtext=fontfile=DTM-Sans.otf:fontsize=100:fontcolor=white:x=w/4-tw/2:y=h/2-th:text='"'Total time\\: $(printf "%d\\\\:%02d\\\\:%02d.%02d" "$hrs" "$min" "$sec" "$csec")'"',drawtext=fontfile=DTM-Sans.otf:fontsize=100:fontcolor=white:x=w/4-tw/2:y=h/2+th/2:text='"'Rerecord count\\: $rerecord_count'"',format=pix_fmts=monob,scale=size=640x120:flags=neighbor' -frames 1 -y tas-splash-2.png

ffmpeg -f concat -i undertale-segments.txt -f concat -i deltarune-segments.txt -f rawvideo -pix_fmt rgba -s 640x120 -r 30 -i <(go run readout.go) -f lavfi -i 'smptebars=size=640x480:rate=30,format=yuv444p,drawtext=fontfile=DTM-Mono.otf:text=%{n}:fontcolor=white:fontsize=48:borderw=4:x=w-tw-8:y=h-th-8,drawtext=fontfile=DTM-Mono.otf:text=No Signal:fontcolor=white:fontsize=72:borderw=4:x=w/2-tw/2:y=h/3-th/3' -filter_complex '[3:v] split [ns1][ns2]; [0:v][ns1] overlay=shortest=1:x=1/between(n\,157518\,157533)-1:format=yuv444 [left]; [1:v][ns2] overlay=shortest=1:x=1/between(n\,65913\,65928)-1:format=yuv444  [right]; [2:v] scale=1280:240:flags=neighbor [bottom]; [left][right] hstack [top]; [top][bottom] vstack [vout]; [0:a]asplit=2[u1][u2]; [1:a]asplit=2[d1][d2]; [u2]lowpass[u3]; [d2]lowpass[d3]; [u1][d3]sidechaincompress[u]; [d1][u3]sidechaincompress[d]; [u][d]amerge=inputs=2,pan=stereo|c0<c0+c1|c1<c2+c3,crossfeed [aout]' -map '[vout]' -map '[aout]' -crf 24 -tune animation -preset veryslow -pix_fmt yuv444p -movflags +faststart -y -r 30 tas.mp4
