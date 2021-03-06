## Setup:

- Install Undertale (via Steam, Itch.io, etc.)
- Install Deltarune (see [this Reddit thread](https://www.reddit.com/r/Deltarune/comments/9wizh3/deltarune_running_on_linux_natively/) for information about how to run it natively on Linux)
- Add `blank-splash.png` to Deltarune as `splash.png`. This is required to make libTAS start both games at the same time. Otherwise, Undertale will start approximately 180 frames after Deltarune because libTAS interprets the splash screen (which takes almost no time at all outside of libTAS) as a 6 second delay on Undertale and a single lag frame on Deltarune.
- This TAS requires [a modification to libTAS](https://github.com/BenLubar-PR/libTAS/tree/undertale-tas) in order to run both games simultaneously on the same machine.
- Since it's hard to install libTAS for multiple architectures at the same time, copy Deltarune's `runner` program (64 bit) over Undertale's (32 bit).
- Rename `undertale/runner` to `undertale/undertale-runner` and `deltarune/runner` to `deltarune/deltarune-runner`. This is required to prevent libTAS from loading save states from the wrong game and also for a later part of the TAS.
- Run [`./makeltm.sh undertale.ltm; ./makeltm.sh deltarune.ltm`](makeltm.sh). This will create libTAS movie files from the extracted contents in the [tas](tas) directory.
- In libTAS, set the following options for Undertale/Deltarune:
  - Runtime / `Backup savefiles in memory` should be checked (other options can be set by preference)
  - Input: `Keyboard support` (disable `Mouse support`, only relevant if editing)
- The "game" you'll be running in libTAS is actually a [proxy program](proxy.c) which assists with restarting Undertale during the TAS. (A compiled version is included in this repository.)
  - There should be two copies; one named `undertale/undertale-proxy` and the other named `deltarune/deltarune-proxy`; again, this is to prevent libTAS from being confused.
  - For each copy, set the argument list to `./undertale-runner` or `./deltarune-runner` based on the game.

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
