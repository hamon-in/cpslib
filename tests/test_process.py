import os

import psutil
from pycpslib import lib as P
from pycpslib import ffi

def test_pid_exists(flush):
    cpid = os.getpid()
    peu = psutil.pid_exists(cpid)
    pel = P.pid_exists(cpid)
    assert peu == pel

def test_process(flush):
    cpid = os.getpid()
    psu = psutil.Process(cpid)
    psl = P.get_process(cpid)
    assert psu.pid == psl.pid
    assert psu.ppid() == psl.ppid
    assert psu.name() == ffi.string(psl.name)
    assert psu.exe() == ffi.string(psl.exe)
    assert " ".join(psu.cmdline()) == ffi.string(psl.cmdline)
    assert psu.create_time() == psl.create_time
    uids = psu.uids()
    assert uids.real == psl.uid
    assert uids.effective == psl.euid
    assert uids.saved == psl.suid
    gids = psu.gids()
    assert gids.real == psl.gid
    assert gids.effective == psl.egid
    assert gids.saved == psl.sgid
    assert psu.username() == ffi.string(psl.username)
    assert psu.terminal() == ffi.string(psl.terminal)
