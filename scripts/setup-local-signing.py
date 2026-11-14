#!/usr/bin/env python3
"""Create one per-user development signing identity. Never commits private material."""
import os, json, secrets, subprocess, tempfile, shlex
from pathlib import Path
root=Path.home()/'Library/Application Support/TrackpadCompat/DevelopmentSigning'
root.mkdir(parents=True,exist_ok=True,mode=0o700)
os.chmod(root,0o700)
if (root/'identity.json').exists():
    print('Existing TrackpadCompat signing identity retained.'); raise SystemExit(0)
def run(args,**kw):
    r=subprocess.run(args,stdout=subprocess.PIPE,stderr=subprocess.PIPE,**kw)
    if r.returncode: raise RuntimeError('Signing setup command failed: '+args[0]+' '+args[1]+'; '+r.stderr.decode(errors='replace'))
    return r.stdout
password=secrets.token_urlsafe(40)
pw=root/'keychain-password';pw.write_text(password);os.chmod(pw,0o600)
kc=root/'trackpadcompat-signing.keychain-db'
run(['security','create-keychain','-p',password,str(kc)])
run(['security','set-keychain-settings','-lu','-t','300',str(kc)])
with tempfile.TemporaryDirectory(dir=root) as work:
    work=Path(work);key=work/'key.pem';cert=root/'certificate.pem';p12=work/'identity.p12';passfile=work/'p12-pass';passfile.write_text(secrets.token_urlsafe(40));os.chmod(passfile,0o600)
    run(['openssl','req','-x509','-newkey','rsa:3072','-sha256','-nodes','-days','3650','-subj','/CN=TrackpadCompat Local Development/','-addext','basicConstraints=critical,CA:FALSE','-addext','keyUsage=critical,digitalSignature','-addext','extendedKeyUsage=critical,codeSigning','-keyout',str(key),'-out',str(cert)])
    os.chmod(key,0o600)
    run(['openssl','pkcs12','-export','-inkey',str(key),'-in',str(cert),'-out',str(p12),'-name','TrackpadCompat Local Development','-passout','file:'+str(passfile),'-keypbe','PBE-SHA1-3DES','-certpbe','PBE-SHA1-3DES','-macalg','sha1'])
    run(['security','import',str(p12),'-k',str(kc),'-P',passfile.read_text(),'-T','/usr/bin/codesign','-x'])
    run(['security','set-key-partition-list','-S','apple-tool:,apple:','-s','-k',password,str(kc)])
raw=run(['openssl','x509','-in',str(root/'certificate.pem'),'-noout','-fingerprint','-sha1']).decode().strip().split('=')[-1].replace(':','')
(root/'identity.json').write_text(json.dumps({'identity':raw,'keychain':str(kc),'name':'TrackpadCompat Local Development'},indent=2))
os.chmod(root/'identity.json',0o600)
chains=shlex.split(run(['security','list-keychains','-d','user']).decode())
if str(kc) not in chains: run(['security','list-keychains','-d','user','-s',*chains,str(kc)])
# One-time user authorization, code-signing trust only. No system trust or TLS trust.
run(['security','add-trusted-cert','-r','trustRoot','-p','codeSign','-k',str(kc),str(root/'certificate.pem')])
run(['security','lock-keychain',str(kc)])
print('Created TrackpadCompat Local Development. Private key restricted to signing tools; keychain locked.')
