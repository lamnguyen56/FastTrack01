"""Host test with fake ip/sleep commands; does not alter network interfaces."""
import os
from pathlib import Path
import subprocess
import tempfile

script = Path(__file__).resolve().parents[1] / 'files/setup_can.sh'
with tempfile.TemporaryDirectory(prefix='can-setup-test-') as folder:
    root = Path(folder)
    ip = root / 'ip'
    ip.write_text('''#!/usr/bin/env python3
import os, sys
from pathlib import Path
root = Path(os.environ['TEST_ROOT'])
args = ' '.join(sys.argv[1:])
with (root / 'calls').open('a') as out: out.write(args + '\\n')
if args.startswith('link show'):
    count = int((root / 'count').read_text()) if (root / 'count').exists() else 0
    (root / 'count').write_text(str(count + 1))
    sys.exit(0 if count >= int(os.environ['APPEAR_AFTER']) else 1)
if 'type can' in args and os.environ.get('FAIL_CONFIG') == '1': sys.exit(2)
''')
    ip.chmod(0o755)
    sleep = root / 'sleep'
    sleep.write_text('#!/bin/sh\nexit 0\n')
    sleep.chmod(0o755)
    # Do not inherit target-specific settings from the developer's shell.
    env = dict(os.environ, PATH=str(root) + ':' + os.environ['PATH'],
               TEST_ROOT=str(root), CAN_IF='can0', CAN_BITRATE='500000',
               CAN_RESTART_MS='100', CAN_WAIT_SECONDS='3')
    for appear, fail, expected in [(0, '0', True), (2, '0', True), (99, '0', False), (0, '1', False)]:
        for file in ['calls', 'count']:
            (root / file).unlink(missing_ok=True)
        run = subprocess.run(['sh', str(script)], env=dict(env, APPEAR_AFTER=str(appear), FAIL_CONFIG=fail),
                             capture_output=True, text=True, timeout=10)
        assert (run.returncode == 0) == expected, run.stdout + run.stderr
        calls = (root / 'calls').read_text()
        if expected:
            assert 'bitrate 500000 restart-ms 100' in calls and 'link set dev can0 up' in calls
        else:
            assert 'link set dev can0 up' not in calls
    print('PASS: immediate/late interface, timeout, failed configuration never brings interface up')
