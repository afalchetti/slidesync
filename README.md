# SlideSync

Slide-Video Synchronizer.
Automatic script which, given a presentation video recording and its corresponding slides file,
matches them and transforms the slides file into a synchronized stream (typically a video file)
which progresses at the same time as in the recording. This output is meant to be further mixed
or edited with the original footage, which is easy since there's no aligning required.

### Prerequisites

This package depends on OpenCV 3, wxWidgets 3, OpenGL 3+ and ImageMagick 7 (with 8 bit quantum).

## Authors

* **Angelo Falchetti Pareja** - [afalchetti](https://github.com/afalchetti)

## License

This project is licensed under the Apache License 2.0 - see the [License.md](License.md) file for details.

It links to ImageMagick, which is licensed under its own ImageMagick License, which can be found in
`dep/license/ImageMagick`.
