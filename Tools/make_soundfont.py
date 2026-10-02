#!/usr/bin/env python3
"""Builds the soundfont the synth plays from two upstream General MIDI fonts.

Keeps only the presets a transcription can name: the program the model emits for
each instrument group, plus the Standard drum kit. All come from MuseScore_General
except the piano, which comes from FluidR3Mono: MuseScore_General's piano relies on
SF2 modulators, which TinySoundFont does not implement. The Ogg sample data is
copied byte for byte, and the output is deterministic: its digest is pinned below.

Run by fetch_soundfont.py once the sources are on disk.
"""

import hashlib
import struct
import sys
from dataclasses import dataclass, field
from pathlib import Path

SOUNDFONT_DIR = Path(__file__).resolve().parent.parent / "NeuralNote" / "Assets" / "SoundFont"

MUSESCORE = "MuseScore_General.sf3"
FLUID = "FluidR3Mono_GM.sf3"
OUTPUT = "NeuralNote_GM.sf3"

OUTPUT_SHA256 = "6152b329bfb7c57c73112e26452e6b61909b19df1f146351bab85a5f3b0ea0aa"

DRUM_BANK = 128
STANDARD_KIT = 0

# The program muscriptor.cpp emits for each instrument group, unnamed ones included (auto mode can
# decode those). Program 96 belongs to the drums group, which plays from DRUM_BANK instead.
GROUP_PROGRAMS = (
    0, 2, 8, 16, 24, 26, 29, 32, 33, 40, 41, 42, 43, 46, 47, 48, 50, 52, 55, 56, 57, 58,
    60, 61, 64, 66, 67, 68, 69, 70, 71, 72, 80, 88, 100, 101,
    *range(97, 100), *range(102, 128),
)  # fmt: skip

PIANO = 0

# (source font, bank, preset), in output order.
SELECTION = sorted(
    [(FLUID, 0, PIANO)]
    + [(MUSESCORE, 0, program) for program in GROUP_PROGRAMS if program != PIANO]
    + [(MUSESCORE, DRUM_BANK, STANDARD_KIT)],
    key=lambda entry: (entry[1], entry[2]),
)

INFO = [
    (b"ifil", struct.pack("<HH", 3, 1)),
    (b"isng", b"E-mu 10K2"),
    (b"INAM", b"NeuralNote GM"),
    (b"IENG", b"Frank Wen, Michael Cowgill & S. Christian Collins - see license files for details"),
    (b"ICOP", b"Frank Wen 2000-02, Michael Cowgill 2014-17, S. Christian Collins 2018-20"),
    (
        b"ICMT",
        b"Derived from MuseScore_General v0.2 and FluidR3Mono_GM 2.312, both released under the MIT "
        b"license. See MuseScore_General_License.md and FluidR3Mono_License.md.",
    ),
]

# SF2 2.04 section 7: field layouts of the pdta sub-chunks.
FORMATS = {
    "phdr": "<20sHHHIII",
    "pbag": "<HH",
    "pmod": "<HHhHH",
    "pgen": "<HH",
    "inst": "<20sH",
    "ibag": "<HH",
    "imod": "<HHhHH",
    "igen": "<HH",
    "shdr": "<20sIIIIIBbHH",
}

GEN_INSTRUMENT = 41
GEN_SAMPLE_ID = 53

SAMPLE_MONO = 1
SAMPLE_LINK_MASK = 0x000F
# Set by MuseScore's .sf3 on Ogg Vorbis samples; TinySoundFont tests 0x30.
SAMPLE_COMPRESSED_MASK = 0x0030


@dataclass
class SoundFont:
    pdta: dict[str, list[tuple]]
    smpl: bytes


def _chunks(data: bytes, start: int, end: int):
    while start < end:
        chunk_id, size = data[start : start + 4], struct.unpack_from("<I", data, start + 4)[0]
        yield chunk_id, start + 8, size
        start += 8 + size + (size & 1)


def read(path: Path) -> SoundFont:
    data = path.read_bytes()

    if data[:4] != b"RIFF" or data[8:12] != b"sfbk":
        raise ValueError(f"{path.name}: not a SoundFont")

    pdta: dict[str, list[tuple]] = {}
    smpl = None

    for chunk_id, offset, size in _chunks(data, 12, len(data)):
        if chunk_id != b"LIST":
            continue

        for sub_id, sub_offset, sub_size in _chunks(data, offset + 4, offset + size):
            name = sub_id.decode("ascii")
            body = data[sub_offset : sub_offset + sub_size]

            if name == "smpl":
                smpl = body
            elif name in FORMATS:
                pdta[name] = list(struct.iter_unpack(FORMATS[name], body))

    if smpl is None or set(pdta) != set(FORMATS):
        raise ValueError(f"{path.name}: missing sample data or hydra chunks")

    return SoundFont(pdta, smpl)


@dataclass
class Builder:
    """Accumulates the output hydra, copying each instrument and sample once however often it is used."""

    out: dict[str, list[tuple]] = field(default_factory=lambda: {name: [] for name in FORMATS})
    smpl: bytearray = field(default_factory=bytearray)
    instruments: dict[tuple[str, int], int] = field(default_factory=dict)
    samples: dict[tuple[str, int], int] = field(default_factory=dict)

    def add_preset(self, fonts: dict[str, SoundFont], source: str, bank: int, preset: int) -> None:
        hydra = fonts[source].pdta
        headers = hydra["phdr"]
        index = next(
            (i for i in range(len(headers) - 1) if headers[i][1] == preset and headers[i][2] == bank),
            None,
        )

        if index is None:
            raise ValueError(f"{source}: no preset {bank}:{preset}")

        name, _, _, first_bag, library, genre, morphology = headers[index]
        self.out["phdr"].append((name, preset, bank, len(self.out["pbag"]), library, genre, morphology))
        self._copy_zones(fonts, source, "p", first_bag, headers[index + 1][3], GEN_INSTRUMENT, self._add_instrument)

    def _add_instrument(self, fonts: dict[str, SoundFont], source: str, index: int) -> int:
        key = (source, index)

        if key not in self.instruments:
            self.instruments[key] = len(self.out["inst"])
            hydra = fonts[source].pdta
            name, first_bag = hydra["inst"][index]
            self.out["inst"].append((name, len(self.out["ibag"])))
            self._copy_zones(fonts, source, "i", first_bag, hydra["inst"][index + 1][1], GEN_SAMPLE_ID, self._add_sample)

        return self.instruments[key]

    def _add_sample(self, fonts: dict[str, SoundFont], source: str, index: int) -> int:
        key = (source, index)

        if key not in self.samples:
            font = fonts[source]
            name, start, end, loop_start, loop_end, rate, pitch, correction, link, kind = font.pdta["shdr"][index]

            # In an .sf3, start and end are byte offsets of the Ogg stream and the loop points are frames
            # relative to its decoded start, so moving the stream only moves start and end.
            if not kind & SAMPLE_COMPRESSED_MASK:
                raise ValueError(f"{source}: sample {index} is not Ogg-compressed")

            self.samples[key] = len(self.out["shdr"])
            new_start = len(self.smpl)
            self.smpl += font.smpl[start:end]
            self.out["shdr"].append(
                (name, new_start, len(self.smpl), loop_start, loop_end, rate, pitch, correction, link, kind)
            )

        return self.samples[key]

    def _copy_zones(self, fonts, source, prefix, first_bag, end_bag, index_generator, add_child) -> None:
        hydra = fonts[source].pdta
        bags, gens, mods = hydra[prefix + "bag"], hydra[prefix + "gen"], hydra[prefix + "mod"]
        out_bags, out_gens, out_mods = self.out[prefix + "bag"], self.out[prefix + "gen"], self.out[prefix + "mod"]

        for bag in range(first_bag, end_bag):
            out_bags.append((len(out_gens), len(out_mods)))

            for operator, amount in gens[bags[bag][0] : bags[bag + 1][0]]:
                if operator == index_generator:
                    amount = add_child(fonts, source, amount)

                out_gens.append((operator, amount))

            # Modulators refer to each other by position within the zone, so they copy unchanged.
            out_mods.extend(mods[bags[bag][1] : bags[bag + 1][1]])

    def finish(self, sources: dict[int, str]) -> None:
        """Appends the terminal records and points sample links at the copies of their partners."""
        shdr = self.out["shdr"]

        for new_index, record in enumerate(shdr):
            name, start, end, loop_start, loop_end, rate, pitch, correction, link, kind = record
            partner = self.samples.get((sources[new_index], link)) if kind & SAMPLE_LINK_MASK != SAMPLE_MONO else None

            if partner is None:
                # Mono, or the other half of a stereo pair was not kept.
                link, kind = 0, SAMPLE_MONO | (kind & ~SAMPLE_LINK_MASK)
            else:
                link = partner

            shdr[new_index] = (name, start, end, loop_start, loop_end, rate, pitch, correction, link, kind)

        out = self.out
        out["phdr"].append((b"EOP", 0, 0, len(out["pbag"]), 0, 0, 0))
        out["pbag"].append((len(out["pgen"]), len(out["pmod"])))
        out["pmod"].append((0, 0, 0, 0, 0))
        out["pgen"].append((0, 0))
        out["inst"].append((b"EOI", len(out["ibag"])))
        out["ibag"].append((len(out["igen"]), len(out["imod"])))
        out["imod"].append((0, 0, 0, 0, 0))
        out["igen"].append((0, 0))
        out["shdr"].append((b"EOS", 0, 0, 0, 0, 0, 0, 0, 0, 0))


def _chunk(chunk_id: bytes, body: bytes) -> bytes:
    return chunk_id + struct.pack("<I", len(body)) + body + (b"\0" if len(body) & 1 else b"")


def _list(list_type: bytes, chunks: list[bytes]) -> bytes:
    return _chunk(b"LIST", list_type + b"".join(chunks))


def _info_string(text: bytes) -> bytes:
    # Zero-terminated, padded to an even length.
    return text + b"\0" * (2 - len(text) % 2)


def build(directory: Path = SOUNDFONT_DIR) -> Path:
    fonts = {name: read(directory / name) for name in (MUSESCORE, FLUID)}
    builder = Builder()

    for source, bank, preset in SELECTION:
        builder.add_preset(fonts, source, bank, preset)

    builder.finish({index: source for (source, _), index in builder.samples.items()})

    # TinySoundFont does not skip RIFF pad bytes, so no chunk may need one. Past the last Ogg stream,
    # this byte belongs to no sample.
    if len(builder.smpl) & 1:
        builder.smpl += b"\0"

    info = [_chunk(cid, body if cid == b"ifil" else _info_string(body)) for cid, body in INFO]
    pdta = [_chunk(name.encode(), b"".join(struct.pack(FORMATS[name], *r) for r in records))
            for name, records in builder.out.items()]  # fmt: skip

    body = b"sfbk" + _list(b"INFO", info) + _list(b"sdta", [_chunk(b"smpl", bytes(builder.smpl))]) + _list(b"pdta", pdta)

    output = directory / OUTPUT
    partial = output.with_suffix(output.suffix + ".partial")
    partial.write_bytes(_chunk(b"RIFF", body))
    partial.replace(output)

    print(f"{OUTPUT}: {len(SELECTION)} presets, {len(builder.instruments)} instruments, "
          f"{len(builder.samples)} samples, {output.stat().st_size / 1e6:.1f} MB")  # fmt: skip

    return output


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def build_and_verify(directory: Path = SOUNDFONT_DIR) -> bool:
    """Builds the font and checks it against the pinned digest.

    A mismatch means the selection, the writer or a source changed: re-pin deliberately.
    """
    actual = sha256(build(directory))

    if actual != OUTPUT_SHA256:
        print(f"{OUTPUT}: expected sha256 {OUTPUT_SHA256}, got {actual}", file=sys.stderr)
        return False

    return True


def main() -> int:
    return 0 if build_and_verify() else 1


if __name__ == "__main__":
    sys.exit(main())
