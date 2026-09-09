"""Measure actual isolated mixer recordings, including the real Coastal class mixes."""
import json
from pathlib import Path
import numpy as np
import soundfile as sf

ROOT = Path(__file__).resolve().parents[2] / 'local-evidence'


def main():
    metrics = {}
    labels = ('music-baseline', 'music-muted', 'music-half', 'master-muted',
              'thunder-baseline', 'thunder-effects-muted', 'thunder-master-muted')
    for label in labels:
        data, rate = sf.read(ROOT / ('m3-soundscape-' + label + '.wav'), always_2d=True)
        if not np.isfinite(data).all() or len(data) < rate:
            raise ValueError('Invalid mixer capture: ' + label)
        metrics[label] = {'rms': float(np.sqrt(np.mean(data ** 2))),
                          'peak': float(np.abs(data).max()), 'seconds': len(data) / rate}
    ratio = metrics['music-half']['rms'] / max(metrics['music-baseline']['rms'], 1e-12)
    checks = {
        'score_audible_signal': metrics['music-baseline']['rms'] > 1e-5,
        'score_50_percent': .47 < ratio < .53,
        'thunder_signal_with_music_muted': metrics['thunder-baseline']['rms'] > 1e-5,
    }
    for label in ('music-muted', 'master-muted', 'thunder-effects-muted', 'thunder-master-muted'):
        checks[label] = metrics[label]['peak'] <= 1e-7
    result = {'passed': all(checks.values()), 'checks': checks, 'metrics': metrics,
              'music_half_rms_ratio': ratio,
              'scope': 'Recorded mixer signal through actual class routes; no listening or physical-device acceptance'}
    (ROOT / 'm3-soundscape-output.json').write_text(json.dumps(result, indent=2))
    print(json.dumps(result, indent=2))
    if not result['passed']:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
