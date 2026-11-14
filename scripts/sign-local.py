#!/usr/bin/env python3
"""Sign using the persistent local identity; never log password or subprocess argv."""
import json, subprocess, sys
from pathlib import Path
root=Path.home()/'Library/Application Support/TrackpadCompat/DevelopmentSigning'
if not (root/'identity.json').exists():
    sys.exit('Run python3 scripts/setup-local-signing.py once before building.')
info=json.loads((root/'identity.json').read_text());kc=info['keychain']
def run(args):
    r=subprocess.run(args,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    if r.returncode: raise RuntimeError(r.stderr.decode(errors='replace'))
    return r
try:
    run(['security','unlock-keychain','-p',(root/'keychain-password').read_text(),kc])
    app=Path(sys.argv[1]).resolve()
    for target in [app/'Contents/Resources/usb-helper',app/'Contents/Resources/bt-spoof',app]:
        run(['codesign','--force','--sign',info['identity'],'--keychain',kc,'--timestamp=none',str(target)])
    run(['codesign','--verify','--strict',str(app)])
    print('Signed and verified with TrackpadCompat Local Development.')
finally:
    subprocess.run(['security','lock-keychain',kc],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
