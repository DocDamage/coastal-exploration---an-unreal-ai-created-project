import runpy
from pathlib import Path
folder = Path(__file__).resolve().parent
runpy.run_path(str(folder / 'repair_m3_surface_normals.py'), run_name='repair')
runpy.run_path(str(folder / 'capture_m3_environment.py'), run_name='capture', init_globals={
    'capture_prefix': 'm3-surfaces-final', 'extra_views': [('overlook', (7050, 7750, 1550), (-19, 37, 0))]})
