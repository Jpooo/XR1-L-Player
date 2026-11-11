#!/usr/bin/env python3
"""Exercise menu navigation and appearance controls through the running SDL app."""
from pathlib import Path
import array,os,re,subprocess,tempfile,sys
root=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='xr1-appearance-') as folder:
    env=os.environ.copy();env.update(SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='disk',
        SDL_DISKAUDIOFILE=folder+'/audio.raw',XDG_CONFIG_HOME=folder,
        LD_LIBRARY_PATH=str(root/'.deps/root/usr/lib/x86_64-linux-gnu'),
        LD_PRELOAD=str(root/'build/controls-probe.so'),XR1_TEST_APPEARANCE='1')
    command=sys.argv[1:] or [str(root/'build/ui-fixture-player')]
    run=subprocess.run([*command,'--start','--seconds','4','--screenshot',str(root/'build/linux-appearance-panel.bmp')],cwd=root,env=env,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=25)
    print(run.stdout);assert run.returncode==0
    config=Path(folder)/'xr1-l-player'
    assert 'theme "playstation"' in (config/'settings').read_text()
    assert not list(config.glob('*fps-test*'))
    for color in ('Neon Green','Electric Blue','Crimson Red'):assert 'Color: '+color in run.stdout
    assert 'FPS report' not in run.stdout
    received,shown=map(int,re.search(r'Frames received: (\d+) / displayed: (\d+)',run.stdout).groups())
    # Modal software rendering is a functional check, not a GPU benchmark.
    assert received>=100 and shown>=30,(received,shown)
    assert 'Audio: 48 kHz stereo / output period 128 samples' in run.stdout
    samples=array.array('h',Path(folder+'/audio.raw').read_bytes())
    assert samples[::2]==samples[1::2]
    assert 1250<=max(abs(s) for s in samples[-24000:])<=1265
    print('PASS all themes, saved preferences, submenu navigation, continued video/audio and unchanged Linux audio period')
