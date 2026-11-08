#!/usr/bin/env python3
"""Generates the 8-bit sound effects and the music loop of the R-Type game.

Usage: python3 tools/generate_audio.py [output directory]
(default: assets/audio). Only the Python standard library is used: the
sounds are square waves and noise, written as 22050 Hz mono 16-bit WAV.
The output is deterministic, so running it again gives the same files.
"""

import math
import random
import struct
import sys
import wave
from pathlib import Path

RATE = 22050


def square(phase, duty=0.5):
    return 1.0 if (phase % 1.0) < duty else -1.0


def write(path, samples, volume=0.8):
    peak = max(1e-9, max(abs(s) for s in samples))
    scale = volume / peak
    frames = b"".join(
        struct.pack("<h", int(max(-1.0, min(1.0, s * scale)) * 32767))
        for s in samples)
    with wave.open(str(path), "wb") as out:
        out.setnchannels(1)
        out.setsampwidth(2)
        out.setframerate(RATE)
        out.writeframes(frames)


def sweep(start, end, seconds, duty=0.5, curve=2.0):
    """A square wave sliding from start to end Hz, fading out."""
    count = int(RATE * seconds)
    phase = 0.0
    samples = []
    for i in range(count):
        t = i / count
        freq = start + (end - start) * t
        phase += freq / RATE
        samples.append(square(phase, duty) * (1.0 - t) ** curve)
    return samples


def noise(seconds, rng, curve=1.5, smooth=0.0):
    """White noise fading out; smooth in [0, 1) makes it duller."""
    count = int(RATE * seconds)
    value = 0.0
    samples = []
    for i in range(count):
        t = i / count
        value = smooth * value + (1.0 - smooth) * rng.uniform(-1.0, 1.0)
        samples.append(value * (1.0 - t) ** curve)
    return samples


def mix(*tracks):
    length = max(len(track) for track in tracks)
    return [sum(track[i] for track in tracks if i < len(track))
            for i in range(length)]


def arpeggio(notes, note_seconds, duty=0.25):
    samples = []
    for freq in notes:
        count = int(RATE * note_seconds)
        phase = 0.0
        for i in range(count):
            phase += freq / RATE
            samples.append(square(phase, duty) * (1.0 - i / count) ** 0.5)
    return samples


def note(name):
    """Frequency of a note such as 'A4' or 'C#3'."""
    names = {"C": -9, "C#": -8, "D": -7, "D#": -6, "E": -5, "F": -4,
             "F#": -3, "G": -2, "G#": -1, "A": 0, "A#": 1, "B": 2}
    key, octave = name[:-1], int(name[-1])
    return 440.0 * 2 ** ((names[key] + 12 * (octave - 4)) / 12)


def music(rng):
    """A chiptune loop: bass, lead and hi-hat, 8 bars of 4 beats."""
    bpm = 140
    beat = 60.0 / bpm
    step = beat / 2
    chords = ["A2", "F2", "C3", "G2"] * 2
    lead = ["E5", "C5", "A4", "C5", "E5", "G5", "E5", "D5",
            "C5", "A4", "F4", "A4", "C5", "D5", "C5", "A4",
            "G4", "C5", "E5", "G5", "E5", "C5", "G4", "C5",
            "D5", "B4", "G4", "B4", "D5", "G5", "F5", "D5"]
    total = int(RATE * beat * 4 * len(chords))
    out = [0.0] * total
    for bar, root in enumerate(chords):
        base = note(root)
        for eighth in range(8):
            start = int(RATE * (bar * 4 * beat + eighth * step))
            freq = base * (2 if eighth % 2 else 1)
            count = int(RATE * step * 0.9)
            phase = 0.0
            for i in range(count):
                phase += freq / RATE
                out[start + i] += 0.35 * square(phase, 0.5) * (1 - i / count)
        # Hi-hat on every eighth, kick-like noise on beats 1 and 3.
        for eighth in range(8):
            start = int(RATE * (bar * 4 * beat + eighth * step))
            hat = noise(0.04, rng, curve=3.0)
            for i, value in enumerate(hat):
                out[start + i] += 0.12 * value
    for index, name in enumerate(lead * 2):
        start = int(RATE * index * beat)
        if start >= total:
            break
        freq = note(name)
        count = int(RATE * beat * 0.85)
        phase = 0.0
        for i in range(count):
            phase += freq / RATE
            vibrato = 1.0 + 0.004 * math.sin(2 * math.pi * 6 * i / RATE)
            phase += freq * (vibrato - 1.0) / RATE
            out[start + i] += 0.28 * square(phase, 0.25) * (1 - i / count) ** 0.3
    return out


def main():
    output = Path(sys.argv[1] if len(sys.argv) > 1 else "assets/audio")
    output.mkdir(parents=True, exist_ok=True)
    rng = random.Random(42)
    write(output / "laser.wav", sweep(1400, 350, 0.12, duty=0.25), 0.5)
    write(output / "charged.wav",
          mix(sweep(300, 90, 0.45, duty=0.5, curve=1.0),
              [0.4 * s for s in noise(0.45, rng, smooth=0.5)]), 0.7)
    write(output / "explosion.wav", noise(0.5, rng, curve=1.2, smooth=0.7), 0.8)
    write(output / "hit.wav", sweep(700, 80, 0.4, duty=0.5, curve=0.8), 0.7)
    write(output / "powerup.wav",
          arpeggio([note(n) for n in ("C5", "E5", "G5", "C6")], 0.07), 0.6)
    write(output / "boss.wav",
          arpeggio([note(n) for n in ("A3", "E3", "A3", "E3")], 0.2, duty=0.5),
          0.7)
    write(output / "stage1.wav", music(rng), 0.6)


if __name__ == "__main__":
    main()
