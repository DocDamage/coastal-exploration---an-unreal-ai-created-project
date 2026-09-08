"""Apply and capture the surface/rail pass without gameplay or native compilation."""
import runpy
from pathlib import Path
folder = Path(__file__).resolve().parent
runpy.run_path(str(folder / 'polish_m3_surfaces.py'), run_name='polish')
runpy.run_path(str(folder / 'capture_m3_environment.py'), run_name='capture', init_globals={
    'capture_prefix': 'm3-surfaces', 'extra_views': [('overlook', (7050, 7750, 1550), (-19, 37, 0))]})
