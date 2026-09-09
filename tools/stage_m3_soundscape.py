"""Decode only selected private music/thunder sources; retain originals and conversion evidence."""
import hashlib
import io
import json
from pathlib import Path
import zipfile
import numpy as np
import soundfile as sf
from stage_m3_interaction_audio import write_once

REPO = Path(__file__).resolve().parents[1]
STAGE = REPO.parent / 'LocalVendor/M3Soundscape'


def main():
    manifest = json.loads((REPO / 'data/m3_soundscape.json').read_text())
    if STAGE.resolve() != STAGE.absolute():
        raise RuntimeError('Independent private staging required')
    rows = []
    for clip in manifest['clips']:
        archive = REPO.parent / 'even newer assets and animations' / clip['archive']
        with zipfile.ZipFile(archive) as source:
            payload = source.read(clip['member'])
            samples, rate = sf.read(io.BytesIO(payload), dtype='float64', always_2d=True)
            if samples.shape[1] not in (1, 2) or not np.isfinite(samples).all():
                raise ValueError('Invalid audio channels or signal')
            peak = float(np.abs(samples).max())
            rms = float(np.sqrt(np.mean(samples ** 2)))
            if not 0 < rms or peak >= 1 or not 0 < len(samples) / rate <= 600:
                raise ValueError('Silent, clipped or overlong source')
            boundary = float(np.max(np.abs(samples[0] - samples[-1])))
            # Short edge ramps remove discontinuities without changing musical timing or loop length.
            if clip['loop']:
                edge = min(round(rate * .01), len(samples) // 4)
                ramp = np.linspace(0, 1, edge)[:, None]
                samples[:edge] *= ramp
                samples[-edge:] *= ramp[::-1]
            buffer = io.BytesIO()
            sf.write(buffer, samples, rate, format='WAV', subtype='PCM_16')
            converted = buffer.getvalue()
            target = STAGE / ('SW_M3_' + clip['id'] + '.wav')
            write_once(target, converted)
            write_once(STAGE / 'Originals' / (clip['id'] + Path(clip['member']).suffix), payload)
            for name in source.namelist():
                if Path(name).suffix.lower() in ('.txt', '.md', '.csv'):
                    write_once(STAGE / 'Documentation' / archive.stem / Path(name).name, source.read(name))
            rows.append(dict(clip, source=str(target), sha256=hashlib.sha256(converted).hexdigest(),
                original_sha256=hashlib.sha256(payload).hexdigest(), sample_rate=rate,
                channels=samples.shape[1], duration=len(samples)/rate, peak=peak, rms=rms,
                original_boundary_step=boundary,
                conversion='PCM16 stereo/mono; music only: 10ms ramps to zero at loop edges'))
    report = {'clips': rows, 'count': len(rows), 'listening_acceptance': 'not_run'}
    (REPO.parent / 'local-evidence/m3-soundscape-staging.json').write_text(json.dumps(report, indent=2))
    print(json.dumps({'staged': len(rows), 'durations': {r['id']: r['duration'] for r in rows}}))


if __name__ == '__main__':
    main()
