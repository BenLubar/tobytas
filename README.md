## Setup:

- Install Undertale (via Steam, Itch.io, etc.)
- Install Deltarune (see [this Reddit thread](https://www.reddit.com/r/Deltarune/comments/9wizh3/deltarune_running_on_linux_natively/) for information about how to run it natively on Linux)
- If you have an Undertale and/or Deltarune save file, back up and remove them. You can do this by simply renaming the folders.
- Assuming you don't want to try to deal with two copies of libTAS for different architectures, do ONE of the following:
  - for 64-bit libTAS, copy the Deltarune runner executable to Undertale.
  - for 32-bit libTAS, copy the Undertale runner executable to Deltarune.
- Run [`./makeltm.sh filename.ltm`](makeltm.sh). This will create a libTAS movie file from the extracted contents in the [tas](tas) directory.
- If you are planning to edit the TAS, rename one or both of the runner executables so that libTAS doesn't get confused.
- In libTAS, set the following options for Undertale/Deltarune:
  - Runtime / `Backup savefiles in memory` (other options can be set by preference)
  - Input: `Keyboard support` (disable `Mouse support`, only relevant if editing)

## Dumping:

- Filename should be `undertale.mp4` or `deltarune.mp4` in this directory.
- ffmpeg settings: `-c:v libx264 -crf 0 -c:a flac -strict -2 -movflags +faststart` (you can use whatever you want; these are just my settings)
- Record movie as usual for libTAS.
  - Tools / `Start encode`
  - Make sure "Movie recording" is enabled (otherwise the game will not receive any inputs at all)
  - General Options / `Pause` must be unchecked or the TAS will wait on frame 1 for input.
  - General Options / `Fast-forward` can be checked to remove the wait time between frames.
  - Click `Start`.
  - Wait for the video to play through, then click `Stop`.

## Encoding:

- `undertale.mp4` and `deltarune.mp4` should already be in this directory.
- If you do not have a Go compiler installed:
  - Get someone else to compile [`readout.go`](readout.go) for you.
  - Edit [`encode.sh`](encode.sh) to call the compiled executable instead of `go run readout.go`.
- Run [`./encode.sh`](encode.sh).

## Editing:

- Use [`./makeltm.sh filename.ltm`](makeltm.sh) to pack the `tas` directory into a libTAS movie.
- Edit the movie as it would be normally done in libTAS.
- Use [`./commit.sh filename.ltm`](commit.sh) to store your changes in Git. This will open your text editor for a commit message.
