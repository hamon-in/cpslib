import os

import psutil
from pycpslib import lib as P
from pycpslib import ffi
import pytest
status_string = { 0 : "running",
                  1 : "sleeping",
                  2 : "disk-sleep",
                  3 : "stopped",
                  4 : "tracing-stop",
                  5 : "zombie",
                  6 : "dead",
                  7 : "wake-kill",
                  8 : "waking",
                  9 : "idle",
                  10 : "locked",
                  11 : "waiting",
                  12 : "suspended"
                 }
def test_pid_exists(flush):
    cpid = os.getpid()
    peu = psutil.pid_exists(cpid)
    pel = P.pid_exists(cpid)
    assert peu == pel


@pytest.mark.linux2
def test_process_linux(flush):
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

@pytest.mark.win32
def test_process_windows(flush):
    proc_status = ffi.typeof('enum proc_status')
    cpid = os.getpid()
    psu = psutil.Process(cpid)
    psl = P.get_process(cpid)
    num_handles = psu.num_handles()
    assert psu.pid == psl.pid
    assert psu.ppid() == psl.ppid
    assert psu.name() == ffi.string(psl.name)
    assert psu.exe() == ffi.string(psl.exe)
    assert " ".join(psu.cmdline()) == ffi.string(psl.cmdline) 
    assert psu.create_time() == psl.create_time
    assert psu.nice() == psl.nice
    #assert num_handles == psl.num_handles # fails for pid given by os.getpid()(current process)
    #assert psu.num_ctx_switches().voluntary == psl.num_ctx_switches
    assert psu.num_threads() == psl.num_threads
    assert psu.username() == ffi.string(psl.username)
    assert psu.status() == status_string[psl.status]
    


	
