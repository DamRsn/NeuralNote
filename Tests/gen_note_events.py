#!/usr/bin/env python

# encoding: utf-8
#
# Copyright 2022 Spotify AB
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# NOTES(@tiborvass): Modified from basic_pitch/note_creation.py

from basic_pitch import note_creation as nc
import json
import numpy as np
import sys

def np_encoder(object):
    if isinstance(object, np.generic):
        return object.item()

def safe_divide(a, b):
    assert(a % b == 0)
    return a // b

if len(sys.argv) != 6:
    print("Usage: %s <input.json> <notes.csv> <onsets.csv> <contours.csv> <output.json>" % sys.argv[0])
    print("""
Structure of <input.json>: [{
    "numFrames": <int>,
    "onsetThreshold": <0.0..1.0>,
    "frameThreshold": <0.0..1.0>,
    "minNoteLength": <int>,
    "minFrequency": <int>,
    "maxFrequency": <int>,
    "pitchBend": <"none", "single", "multi">,
    "melodiaTrick": true,
    "energyThreshold": <int>,
    "inferOnsets": true
}, ...]""")
    sys.exit(1)

# Opening JSON file
with open(sys.argv[1], 'r') as openfile:
    all_cases = json.load(openfile)

result = []

for params in all_cases:
    n_frames = int(params["numFrames"])

    pitch_bend = params.get("pitchBend", None)

    if pitch_bend == None:
        include_pitch_bends = False
    elif pitch_bend == "multi":
        include_pitch_bends = True
        multiple_pitch_bends = True
    elif pitch_bend == "single" or pitch_bend == "":
        include_pitch_bends = True
        multiple_pitch_bends = False
    else:
        raise Exception("Unknown pitchBend %s" % pitch_bend)

    frames = np.genfromtxt(sys.argv[2], dtype=float)
    onsets = np.genfromtxt(sys.argv[3], dtype=float)
    contours = np.genfromtxt(sys.argv[4], dtype=float)

    frames = frames.reshape(n_frames, safe_divide(frames.shape[0],n_frames))
    onsets = onsets.reshape(n_frames, safe_divide(onsets.shape[0],n_frames))
    contours = contours.reshape(n_frames, safe_divide(contours.shape[0],n_frames))
    min_freq = params.get("minFrequency", None)
    max_freq = params.get("maxFrequency", None)

    estimated_notes = nc.output_to_notes_polyphonic(
        frames,
        onsets,
        onset_thresh=params["onsetThreshold"],
        frame_thresh=params["frameThreshold"],
        infer_onsets=params["inferOnsets"],
        min_note_len=params["minNoteLength"],
        min_freq=min_freq if min_freq is not None and min_freq >= 0 else None,
        max_freq=max_freq if max_freq is not None and max_freq >= 0 else None,
        melodia_trick=params["melodiaTrick"],
        energy_tol=params["energyThreshold"],
    )

    if include_pitch_bends:
        estimated_notes_with_pitch_bend = nc.get_pitch_bends(contours, estimated_notes)
    else:
        estimated_notes_with_pitch_bend = [(note[0], note[1], note[2], note[3], None) for note in estimated_notes]

    if include_pitch_bends and not multiple_pitch_bends:
        estimated_notes_with_pitch_bend = nc.drop_overlapping_pitch_bends(estimated_notes_with_pitch_bend)

    times_s = nc.model_frames_to_time(contours.shape[0])

    # transform to dictionary
    result.append([
        {"startTime": times_s[x[0]], "endTime": times_s[x[1]], "startFrame": x[0], "endFrame": x[1], "pitch": x[2], "amplitude": x[3] }
        | ({} if x[4] == None else {"bends": x[4]})  # only add "bends" key if it is not null
        for x in sorted(estimated_notes_with_pitch_bend) # sort in all cases, contrary to original basic pitch implementation
    ])

dump = json.dumps(result, default=np_encoder, separators=(',', ':'))
with open(sys.argv[5], "w") as outfile:
    outfile.write(dump)
