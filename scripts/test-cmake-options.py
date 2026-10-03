"""Exercise the actual top-level option block in fresh CMake caches."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'CMakeLists.txt').read_text(encoding='utf-8')
# Isolate options from CEF downloads and target builds, while using the actual
# option declarations/compatibility logic rather than a copied implementation.
option_block = source.split('add_library(', 1)[0]
cases = [
    ('defaults', [], {'KINGFN_BUILD_TESTS': 'ON', 'KINGFN_ENABLE_CEF': 'OFF', 'USE_SANDBOX': 'ON'}),
    ('legacy', ['-DBRAVELIKE_BUILD_TESTS=OFF', '-DBRAVELIKE_ENABLE_CEF=ON'], {'KINGFN_BUILD_TESTS': 'OFF', 'KINGFN_ENABLE_CEF': 'ON'}),
    ('explicit-wins', ['-DBRAVELIKE_BUILD_TESTS=OFF', '-DKINGFN_BUILD_TESTS=ON', '-DBRAVELIKE_ENABLE_CEF=ON', '-DKINGFN_ENABLE_CEF=OFF'], {'KINGFN_BUILD_TESTS': 'ON', 'KINGFN_ENABLE_CEF': 'OFF'}),
]
with tempfile.TemporaryDirectory(prefix='kingfn-options-') as temporary:
    temporary = Path(temporary)
    fixture = temporary / 'source'
    fixture.mkdir()
    (fixture / 'CMakeLists.txt').write_text(option_block, encoding='utf-8')
    for name, arguments, expected in cases:
        build = temporary / name
        subprocess.run(['cmake', '-S', str(fixture), '-B', str(build), *arguments], check=True)
        values = {}
        for line in (build / 'CMakeCache.txt').read_text(encoding='utf-8').splitlines():
            if line.startswith(('KINGFN_', 'USE_SANDBOX:')) and '=' in line:
                key, value = line.split('=', 1)
                values[key.split(':', 1)[0]] = value
        for key, value in expected.items():
            assert values.get(key) == value, (name, key, values.get(key), value)
        print('PASS:', name)
