"""Stage three semantically supported combat cues privately; do not stage fishing without events."""
import hashlib
import io
import json
import math
import struct
import wave
import zipfile
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
ROOT = REPO.parent
STAGE = ROOT / 'LocalVendor/M3CombatAudio'
REPORT = ROOT / 'local-evidence/m3-combat-audio-staging.json'


def inspect(payload):
    with wave.open(io.BytesIO(payload), 'rb') as source:
        channels, width, rate, frames, compression, _ = source.getparams()
        if (channels, width, rate, compression) != (2, 2, 48000, 'NONE') or frames <= 0:
            raise RuntimeError('Expected nonempty 48 kHz stereo PCM16')
        samples = struct.unpack('<' + 'h' * frames * channels, source.readframes(frames))
    peak = max(abs(value) for value in samples) / 32768
    rms = math.sqrt(sum(value * value for value in samples) / len(samples)) / 32768
    if rms <= 0 or peak >= 1:
        raise RuntimeError('Selected combat cue is silent or clipped')
    return {'channels': channels, 'sample_rate': rate, 'duration': frames / rate,
            'peak_dbfs': 20 * math.log10(peak), 'rms_dbfs': 20 * math.log10(rms)}


def write_once(path, payload):
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists() and path.read_bytes() != payload:
        raise RuntimeError('Refusing to overwrite changed private staging: ' + str(path))
    if not path.exists():
        path.write_bytes(payload)


def main():
    spec = json.loads((REPO / 'data/m3_combat_fishing_audio.json').read_text())
    combat = spec['combat']
    archive = ROOT / 'even newer assets and animations' / combat['archive']
    rows = []
    with zipfile.ZipFile(archive) as source:
        wavs = [i for i in source.infolist() if not i.is_dir() and i.filename.lower().endswith('.wav')]
        if len(wavs) != combat['observed_wav_count']:
            raise RuntimeError('Combat archive WAV inventory changed')
        license_payload = source.read('LICENSE.txt')
        write_once(STAGE / 'Documentation/LICENSE.txt', license_payload)
        for cue in combat['selected']:
            payload = source.read(cue['member'])
            target = STAGE / ('SW_M3Combat_' + cue['id'] + '_' + Path(cue['member']).stem + '.wav')
            write_once(target, payload)
            rows.append(dict(cue, source=str(target), sha256=hashlib.sha256(payload).hexdigest(),
                             **inspect(payload)))
    fishing_archive = ROOT / 'even newer assets and animations' / spec['fishing']['archive']
    with zipfile.ZipFile(fishing_archive) as fishing:
        fishing_count = sum(not i.is_dir() and i.filename.lower().endswith('.wav') for i in fishing.infolist())
    if fishing_count != spec['fishing']['observed_wav_count'] or spec['fishing']['selected']:
        raise RuntimeError('Fishing must remain unselected until authoritative events exist')
    report = {'status': 'combat_selected_staged_listening_pending_fishing_not_staged',
              'archive_sha256': hashlib.sha256(archive.read_bytes()).hexdigest(),
              'license_sha256': hashlib.sha256(license_payload).hexdigest(),
              'combat_wav_count': len(wavs), 'combat': rows,
              'fishing_wav_count': fishing_count, 'fishing_staged': 0}
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
