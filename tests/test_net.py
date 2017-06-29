import psutil
from pycpslib import lib as P
from pycpslib import ffi

def test_net_io_counters(flush):
    psutil_counters = psutil.net_io_counters(True)
    pslib_counter_info = P.net_io_counters()

    for p in range(pslib_counter_info.nitems):
        name = ffi.string(pslib_counter_info.iocounters[p].name)
        bytes_sent = pslib_counter_info.iocounters[p].bytes_sent
        bytes_recv = pslib_counter_info.iocounters[p].bytes_recv
        packets_sent = pslib_counter_info.iocounters[p].packets_sent
        packets_recv = pslib_counter_info.iocounters[p].packets_recv
        errin = pslib_counter_info.iocounters[p].errin
        errout = pslib_counter_info.iocounters[p].errout
        dropin = pslib_counter_info.iocounters[p].dropin
        dropout = pslib_counter_info.iocounters[p].dropout

        assert psutil_counters[name].bytes_sent == bytes_sent
        assert psutil_counters[name].bytes_recv == bytes_recv
        assert psutil_counters[name].packets_sent == packets_sent
        assert psutil_counters[name].packets_recv == packets_recv
        assert psutil_counters[name].errin == errin
        assert psutil_counters[name].errout == errout
        assert psutil_counters[name].dropin == dropin
        assert psutil_counters[name].dropout == dropout
        
def test_number_of_connections(flush):
    kind = ["all", "tcp", "tcp4", "tcp6", "udp", "udp4", "udp6", "unix", "inet", "inet4", "inet6"]
    for each in kind:
        expected_connections = psutil.net_connections(each)
        actual_connections = P.net_connections(each)
        assert actual_connections.nitems == len(expected_connections)


def test_all_net_connection_attribs(flush):
    kind = ["all", "tcp", "tcp4", "tcp6", "udp", "udp4", "udp6", "unix", "inet", "inet4", "inet6"]
    con_status = ffi.typeof('enum con_status')
    for each in kind:
        expected_connections = psutil.net_connections(each)
        actual_connections = P.net_connections(each)
        for i in range(actual_connections.nitems):
            fd = actual_connections.Connections[i].fd
            family = actual_connections.Connections[i].family
            _type = actual_connections.Connections[i].type
            if ffi.string(actual_connections.Connections[i].laddr.ip) == 'NONE':
                laddr = ()
            elif actual_connections.Connections[i].laddr.port == -1:
                laddr = ffi.string(actual_connections.Connections[i].laddr.ip)
            else:
                laddr =  ffi.string(actual_connections.Connections[i].laddr.ip), actual_connections.Connections[i].laddr.port
            if ffi.string(actual_connections.Connections[i].raddr.ip) == 'NONE':
                raddr = ()
            elif actual_connections.Connections[i].raddr.port == -1:
                raddr = ffi.string(actual_connections.Connections[i].raddr.ip)
            else:
                raddr =  ffi.string(actual_connections.Connections[i].raddr.ip), actual_connections.Connections[i].raddr.port
            status = con_status.elements[actual_connections.Connections[i].status]
            pid = actual_connections.Connections[i].pid
            found = False
            for part in expected_connections:
                if all([part.fd == fd,
                        part.family == family,
                        part.type == _type,
                        part.laddr == laddr,
                        part.raddr == raddr,
                        part.status == status,
                        part.pid == pid or (part.pid is None and pid == -1)]):
                    found = True
                    break
            assert found, """No match for conn (fd = '{}',  family= '{}', type= '{}', laddr = '{}', raddr = '{}', status = '{}', pid = '{}')""".format(fd, family, _type, laddr, raddr, status, pid)
