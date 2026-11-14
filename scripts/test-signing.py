#!/usr/bin/env python3
import subprocess,tempfile
from pathlib import Path
source=Path('build/preview/Trackpad Anything.app').resolve()
def run(args):return subprocess.check_output(args,stderr=subprocess.STDOUT,text=True).strip()
def requirement(app):return [s for s in run(['codesign','-d','-r-',str(app)]).splitlines() if s.startswith('designated =>')][0]
def cdhash(app):return [s for s in run(['codesign','-d','--verbose=4',str(app)]).splitlines() if s.startswith('CDHash=')][0]
before=requirement(source);hash_before=cdhash(source)
with tempfile.TemporaryDirectory(prefix='trackpad-signing-test-') as tmp:
 app=Path(tmp)/'Trackpad Anything.app';run(['ditto',str(source),str(app)])
 (app/'Contents/Resources/signing-regression.txt').write_text('Different build; identical app identity\n')
 print(run(['python3','scripts/sign-local.py',str(app)]))
 assert requirement(app)==before;assert cdhash(app)!=hash_before
 run(['codesign','--verify','--strict',str(app)])
print('PASS: changed build hash; identical certificate-based designated requirement; noninteractive signing and verification.')
