[![License: Zlib](https://img.shields.io/badge/License-Zlib-lightgrey.svg)](https://opensource.org/licenses/Zlib)

# Evaluator

Write short C-style programs whose output is used to generate sound!

Evaluator is inspired by bytebeat, a "genre" of music discovered by viznut, which he documented in several youtube videos.

Evaluator's approach, however, is not purist, and its language is not C. The language contains much of the same syntax, most of the operators, and the same operator precedence, but introduces some additional features that make generating musical sounds a little bit easier and also make real-time MIDI control of the program possible. It also includes several built-in presets that demonstrate all the language features, supports save/load to fxp, and loading a program from a plain text file.

The basic idea is that a program operating on 64-bit unsigned integers is used to generate every audio sample. The program essentially runs in a while loop that automatically increments the variables t, m, and q before executing the program code each time.

# How to Build

- clone https://github.com/ddf/wdl-ol
- create a folder named Projects in the root of the repo
- clone this repo into the Projects folder
- to build the VST version, follow the instructions here to install the VST SDK: https://github.com/ddf/wdl-ol/tree/master/VST_SDK
- to build the VST3 version, follow the instructions to install the VST 3.6.6 SDK: https://github.com/ddf/wdl-ol/tree/master/VST3_SDK, it should not be necessary to modify any project files
- open Evaluator.sln in Visual Studio 2015 or Evaluator.xcodeproj in XCode 9 and build the flavor you are interested in
