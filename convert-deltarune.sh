#!/bin/bash -e

rm -rf deltarune
mkdir deltarune deltarune/assets
cp deltarune-runner deltarune/runner
chmod +x deltarune/runner
cp ~/.steam/steam/steamapps/common/DELTARUNEdemo/data.win deltarune/assets/game.unx
cp ~/.steam/steam/steamapps/common/DELTARUNEdemo/audiogroup1.dat deltarune/assets/audiogroup1.dat
cp ~/.steam/steam/steamapps/common/DELTARUNEdemo/snd_* deltarune/assets
cp ~/.steam/steam/steamapps/common/DELTARUNEdemo/AUDIO_INTRONOISE.ogg deltarune/assets/audio_intronoise.ogg
cp ~/.steam/steam/steamapps/common/DELTARUNEdemo/AUDIO_INTRONOISE_ch1.ogg deltarune/assets/audio_intronoise_ch1.ogg
cp -R ~/.steam/steam/steamapps/common/DELTARUNEdemo/{mus,lang} deltarune/assets
mv deltarune/assets/mus/AUDIO_ANOTHERHIM.ogg deltarune/assets/mus/audio_anotherhim.ogg
mv deltarune/assets/mus/AUDIO_DARKNESS.ogg deltarune/assets/mus/audio_darkness.ogg
mv deltarune/assets/mus/AUDIO_DEFEAT.ogg deltarune/assets/mus/audio_defeat.ogg
mv deltarune/assets/mus/AUDIO_DRONE.ogg deltarune/assets/mus/audio_drone.ogg
mv deltarune/assets/mus/AUDIO_STORY.ogg deltarune/assets/mus/audio_story.ogg
mv deltarune/assets/mus/GALLERY.ogg deltarune/assets/mus/gallery.ogg
mv deltarune/assets/mus/KEYGEN.ogg deltarune/assets/mus/keygen.ogg
mv deltarune/assets/mus/THE_HOLY.ogg deltarune/assets/mus/the_holy.ogg
