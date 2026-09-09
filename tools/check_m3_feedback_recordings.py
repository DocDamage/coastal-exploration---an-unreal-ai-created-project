"""Measure the real UE mixer captures; signal checks are separate from listening acceptance."""
import hashlib
import json
import math
import struct
import wave
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2] / 'local-evidence'


def measure(path):
    with wave.open(str(path), 'rb') as stream:
        if stream.getsampwidth() != 2 or stream.getcomptype() != 'NONE':
            raise ValueError('Expected exported PCM16 WAV')
        frames, channels, rate = stream.getnframes(), stream.getnchannels(), stream.getframerate()
        samples = struct.unpack('<' + 'h' * frames * channels, stream.readframes(frames))
    return {'duration': frames / rate, 'channels': channels, 'sample_rate': rate,
            'rms': math.sqrt(sum(s * s for s in samples) / len(samples)) / 32768,
            'peak': max(abs(s) for s in samples) / 32768,
            'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}


def main():
    rows = {name: measure(ROOT / ('m3-feedback-' + name + '.wav')) for name in
            ('baseline', 'effects-muted', 'master-muted', 'effects-half')}
    baseline = rows['baseline']['rms']
    checks = {'baseline_has_signal': baseline > 1e-5,
              'captures_have_expected_duration': all(4 < r['duration'] < 9 for r in rows.values())}
    if baseline > 0:
        for name, row in rows.items():
            row['rms_relative_to_baseline'] = row['rms'] / baseline
        checks.update(effects_mute=rows['effects-muted']['rms'] / baseline < .001,
                      master_mute=rows['master-muted']['rms'] / baseline < .001,
                      effects_half=.4 < rows['effects-half']['rms'] / baseline < .6)
    report = {'passed': all(checks.values()), 'checks': checks, 'captures': rows,
              'scope': 'Actual UE output signal and gain ratios; no listening/physical-speaker acceptance'}
    (ROOT / 'm3-interaction-audio-output.json').write_text(json.dumps(report, indent=2))
    print(json.dumps(report, indent=2))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
