"""Resolve the independent UE5.8 trial after moving to another machine."""
import json
import os
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
WORKSPACE = REPO.parent
TRIAL = WORKSPACE / 'LocalHost58/CoastalExploration'
PROJECT = TRIAL / 'CoastalExploration.uproject'
EVIDENCE = WORKSPACE / 'local-evidence'


def engine_root():
    override = os.environ.get('COASTAL_UE58_ROOT')
    candidates = [Path(override)] if override else []
    if not override and os.name == 'nt':
        import winreg
        try:
            with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE,
                               r'SOFTWARE\EpicGames\Unreal Engine\5.8') as key:
                candidates.append(Path(winreg.QueryValueEx(key, 'InstalledDirectory')[0]))
        except OSError:
            pass
    if not override:
        manifest = Path(os.environ.get('ProgramData', 'C:/ProgramData')) / 'Epic/UnrealEngineLauncher/LauncherInstalled.dat'
        if manifest.is_file():
            for entry in json.loads(manifest.read_text(encoding='utf-8-sig')).get('InstallationList', []):
                if entry.get('AppName') == 'UE_5.8' and entry.get('InstallLocation'):
                    candidates.append(Path(entry['InstallLocation']))
        candidates.append(Path(os.environ.get('ProgramFiles', 'C:/Program Files')) /
                          'Epic Games/UE_5.8')
    for candidate in candidates:
        version = candidate / 'Engine/Build/Build.version'
        if version.is_file():
            data = json.loads(version.read_text(encoding='utf-8-sig'))
            if (data.get('MajorVersion'), data.get('MinorVersion')) == (5, 8):
                return candidate.resolve()
    raise RuntimeError('Set COASTAL_UE58_ROOT to the installed Unreal 5.8 directory')


def validate_trial():
    if not PROJECT.is_file() or TRIAL.resolve() != TRIAL.absolute():
        raise RuntimeError('Expected the independent LocalHost58 project')
    if json.loads(PROJECT.read_text()).get('EngineAssociation') != '5.8':
        raise RuntimeError('Expected EngineAssociation 5.8; original host must stay unchanged')


def build_environment():
    scratch = WORKSPACE / 'Cache/UE58/Temp'
    scratch.mkdir(parents=True, exist_ok=True)
    env = os.environ.copy()
    env.update(TEMP=str(scratch), TMP=str(scratch))
    env['UE-LocalDataCachePath'] = str(WORKSPACE / 'Cache/UE58/DDC')
    env['UBA_ROOT'] = str(WORKSPACE / 'Cache/UE58/UBA')
    return env


if __name__ == '__main__':
    validate_trial()
    print(json.dumps({'project': str(PROJECT), 'engine': str(engine_root()),
                      'evidence': str(EVIDENCE), 'workspace': str(WORKSPACE)}))
