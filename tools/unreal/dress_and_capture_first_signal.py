"""Full editor entry point: assemble and capture without launching gameplay."""
import runpy
from pathlib import Path
folder = Path(__file__).resolve().parent
runpy.run_path(str(folder / 'dress_first_signal.py'), run_name='dressing')
runpy.run_path(str(folder / 'finish_m3_basin.py'), run_name='basin')
runpy.run_path(str(folder / 'capture_m3_environment.py'), run_name='capture')
