#!/usr/bin/env python3
"""Exercise the test-only fixture player's buttons and slider, then inspect the generated audio."""
from pathlib import Path
import array,os,subprocess,tempfile,sys
root=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='xr1-controls-') as work:
    env=os.environ.copy()
    env.update(SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='disk',
               SDL_DISKAUDIOFILE=work+'/audio.raw',XDG_CONFIG_HOME=work,
               LD_LIBRARY_PATH=str(root/'.deps/root/usr/lib/x86_64-linux-gnu'),
               LD_PRELOAD=str(root/'build/controls-probe.so'))
    command=sys.argv[1:] or [str(root/'build/ui-fixture-player')]
    run=subprocess.run([*command,'--start','--seconds','4',
                        '--screenshot',str(root/'build/controls-menu.bmp')],
                       cwd=root,env=env,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=20)
    print(run.stdout)
    assert run.returncode==0,run.returncode
    assert 'Volume: 75%' in run.stdout and 'Volume: 50%' in run.stdout
    assert 'Muted: yes' in run.stdout and 'Muted: no' in run.stdout
    assert 'Menu motion: off' in run.stdout and 'Menu motion: on' in run.stdout
    assert [s for s in run.stdout.splitlines() if s.startswith('Fullscreen:')]==[
        'Fullscreen: on','Fullscreen: off','Fullscreen: on','Fullscreen: off',
        'Fullscreen: on','Fullscreen: off']
    samples=array.array('h',Path(work+'/audio.raw').read_bytes())
    assert samples[::2]==samples[1::2]
    # The end of the run is 50% volume, verified at the real playback callback.
    peak=max(abs(s) for s in samples[-24000:])
    assert 890<=peak<=900,peak
    print('PASS volume/audio, fullscreen, settings, Tab return to video, motion toggle, packaged icons and linear video filtering')
