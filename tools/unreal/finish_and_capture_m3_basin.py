"""Resume the already-dressed coast at the basin construction stage."""
import runpy
from pathlib import Path
folder = Path(__file__).resolve().parent
runpy.run_path(str(folder / 'finish_m3_basin.py'), run_name='basin')
runpy.run_path(str(folder / 'capture_m3_environment.py'), run_name='capture')
