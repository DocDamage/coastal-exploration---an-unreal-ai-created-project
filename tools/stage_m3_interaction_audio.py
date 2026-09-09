"""Stage selected user-supplied WAVs privately, recording exact hashes and signal data."""
import hashlib
import io
import json
import math
import struct
import wave
import zipfile
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
WORKSPACE = REPO.parent
STAGE = WORKSPACE / 'LocalVendor/M3InteractionAudio'


def inspect_pcm(data):
    with wave.open(io.BytesIO(data), 'rb') as source:
        channels, width, rate, frames, compression, _ = source.getparams()
        if width != 2 or channels not in (1, 2) or compression != 'NONE' or frames <= 0:
            raise ValueError('Expected nonempty mono/stereo PCM16')
        samples = struct.unpack('<' + 'h' * (frames * channels), source.readframes(frames))
    peak = max(abs(s) for s in samples) / 32768
    rms = math.sqrt(sum(s * s for s in samples) / len(samples)) / 32768
    if not 0 < frames / rate <= 10 or rms <= 0 or peak >= 1:
        raise ValueError('Empty, overlong or clipped interaction cue')
    return {'channels': channels, 'sample_rate': rate, 'duration': frames / rate,
            'peak_dbfs': 20 * math.log10(peak), 'rms_dbfs': 20 * math.log10(rms)}


def write_once(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists() and path.read_bytes() != data:
        raise RuntimeError('Refusing to overwrite changed private staging: ' + str(path))
    if not path.exists():
        path.write_bytes(data)


def main():
    manifest = json.loads((REPO / 'data/m3_interaction_audio.json').read_text())
    if STAGE.resolve() != STAGE.absolute():
        raise RuntimeError('Private staging must be an independent directory')
    rows = []
    archives = {}
    for clip in manifest['clips']:
        archive = WORKSPACE / 'even newer assets and animations' / clip['archive']
        with zipfile.ZipFile(archive) as source:
            payload = source.read(clip['member'])
            row = dict(clip, **inspect_pcm(payload))
            row['sha256'] = hashlib.sha256(payload).hexdigest()
            target = STAGE / ('SW_M3_' + clip['id'] + '.wav')
            write_once(target, payload)
            row['source'] = str(target)
            rows.append(row)
            if clip['archive'] not in archives:
                archives[clip['archive']] = hashlib.sha256(archive.read_bytes()).hexdigest()
                for info in source.infolist():
                    if Path(info.filename).name.lower() in ('license.txt', 'readme.txt'):
                        write_once(STAGE / 'Documentation' / archive.stem / Path(info.filename).name,
                                   source.read(info))
    report = {'status': 'selected_staged_not_audibly_accepted', 'clips': rows,
              'archives': archives, 'audition': manifest['audition']}
    target = WORKSPACE / 'local-evidence/m3-interaction-audio-staging.json'
    target.write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
