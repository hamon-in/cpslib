import psutil
from pycpslib import lib as P
from pycpslib import ffi


def test_get_users(flush):
    psutil_users = psutil.users()
    pslib_users = P.get_users()

    for i in range(pslib_users.nitems):
        username = ffi.string(pslib_users.users[i].username)
        tty = ffi.string(pslib_users.users[i].tty)
        hostname = ffi.string(pslib_users.users[i].hostname)
        tstamp = pslib_users.users[i].tstamp

        found = False
        for part in psutil_users:
            if all([part.name == username,
                    part.terminal == tty,
                    part.host == hostname or (part.host is None and hostname == ''), # special case for behavior on osx
                    part.started == tstamp]):
                found = True
        assert found, """No match for User(username = '{}', tty = '{}', hostname = '{}', tstamp = '{}')""".format(username, tty, hostname, tstamp)
