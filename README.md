## Setup:

1. Install Undertale and Deltarune Chapter 1 and 2 Demo on Steam on Linux.
2. libTAS 1.4.2 settings required:
    - Runtime > Time tracking > `clock_gettime()`
    - Runtime > Debug > Time tracking all threads > `clock_gettime()`
    - Runtime > Prevent writing to disk
    - Runtime > Virtual Steam client
3. TODO

## Dumping:

- Filename should be `undertale.mp4` or `deltarune.mp4` in this directory.
- ffmpeg settings: `-c:v libx264 -crf 0 -c:a flac -strict -2 -movflags +faststart` (you can use whatever you want; these are just my settings)
- Record movie as usual for libTAS.
  - Tools / `Start encode`
  - Make sure "Movie recording" is enabled (otherwise the game will not receive any inputs at all)
  - General Options / `Pause` must be unchecked or the TAS will wait on frame 1 for input.
  - General Options / `Fast-forward` can be checked to remove the wait time between frames. (This will desync on Deltarune.)
  - Click `Start`.
  - Wait for the video to play through, then click `Stop`.

## Encoding:

- `undertale.mp4`, `undertale_1.mp4`, `deltarune.mp4`, and `deltarune_1.mp4` should already be in this directory.
- If you do not have a Go compiler installed:
  - Get someone else to compile [`readout.go`](readout.go) for you.
  - Edit [`encode.sh`](encode.sh) to call the compiled executable instead of `go run readout.go`.
- Run [`./encode.sh`](encode.sh).

## Editing:

- Use [`./makeltm.sh filename.ltm`](makeltm.sh) to pack the `tas` directory into a libTAS movie.
- Edit the movie as it would be normally done in libTAS.
- Use [`./commit.sh filename.ltm`](commit.sh) to store your changes in Git. This will open your text editor for a commit message.

## Special thanks

- *Determination* fonts from <https://www.behance.net/gallery/31268855/Determination-Better-Undertale-Font>
