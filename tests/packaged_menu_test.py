#!/usr/bin/env python3
"""Check the shipping app's idle menus and removed tester, without fake hardware."""
from pathlib import Path
import os, subprocess, sys, tempfile
root = Path(__file__).resolve().parents[1]
command = sys.argv[1:] or [str(root / 'build/xr1-l-player')]
with tempfile.TemporaryDirectory(prefix='xr1-packaged-') as folder:
    env = os.environ.copy()
    env.update(SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy',
               XDG_CONFIG_HOME=folder,
               LD_LIBRARY_PATH=str(root / '.deps/root/usr/lib/x86_64-linux-gnu'))
    help_run = subprocess.run([*command, '--help'], env=env, capture_output=True, text=True, timeout=15)
    assert help_run.returncode == 0 and '--demo' not in help_run.stdout
    removed = subprocess.run([*command, '--demo'], env=env, capture_output=True, text=True, timeout=15)
    assert removed.returncode != 0 and 'Unknown option: --demo' in removed.stderr
    env.update(LD_PRELOAD=str(root / 'build/controls-probe.so'), XR1_TEST_IDLE='1')
    image = root / 'build/cleanup-v1524/packaged-settings.bmp'
    run = subprocess.run([*command, '--seconds', '2', '--screenshot', str(image)],
                         env=env, capture_output=True, text=True, timeout=20)
    print(run.stdout)
    assert run.returncode == 0, run.stderr
    assert 'Test picture' not in run.stdout and 'TEST PICTURE' not in run.stdout
    assert 'Frames received: 0 / displayed: 0' in run.stdout
    assert 'Fullscreen: on' in run.stdout and 'Fullscreen: off' in run.stdout
    settings = (Path(folder) / 'xr1-l-player/settings').read_text()
    assert 'theme "playstation"' in settings and 'motion "off"' in settings
    assert 'volume "0.750000"' in settings and 'muted "false"' in settings
    assert image.is_file()
    print('PASS shipping menus, saved volume/theme/motion, fullscreen and removed --demo')
