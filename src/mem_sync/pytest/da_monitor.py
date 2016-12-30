# this script is used for monitoring dataid and agents
# it tells which dataid is used by which agents and tells 
# the dataid and versions that used by the given agent.
	
import sys
import os
import json
import getopt
import socket

from kazoo.client import KazooClient
from kazoo.exceptions import NoNodeError, NodeExistsError
error = []

def usage():
    print >> sys.stderr, 'usage: python da_monitor.py --zklist [options] [command]'
    print >> sys.stderr, 'options:'
    print >> sys.stderr, '  --help:    show usage'
    print >> sys.stderr, '  --zklist:  zookeeper\'s ip and port'
    print >> sys.stderr, '  --dataids: a comma separated dataid list that shows\n' + \
                         '                 the newest version of this dataid\n' + \
                         '                 and which agents are used this dataid'
    print >> sys.stderr, '  --agents:  a comma separated list that specifies \n' + \
                         '                 the agent ip, showing the agents'
    print >> sys.stderr, 'command:'
    print >> sys.stderr, '  show: show infomation in zookeeper'
    print >> sys.stderr, '  register: register agents to monitor dataids'
    os._exit(1)

def format_data(data_dict):
    fmt = '{0:>6}{1:>18}{2:>18}{3:>10}{4:>10}{5:>14}{6:>14}{7:>14}'
    print fmt.format(
        'dataid', 'agent address', 'server address', 'cur_ver', \
        'new_ver', 'key(hex)', 'key(dec)', 'size')

    print '-' * (6 + 18 + 18 + 10 + 10 + 14 + 14 + 14)

    fmt = '{0:>6}{1:>18}{2:>18}{3:>10}{4:>10}{5:>14x}{6:>14}{7:>14}'
    for id_, agents in sorted(data_dict.items(), key=lambda x:int(x[0])):
        for agent, ver in agents.iteritems():
            try:
                print fmt.format(id_, agent, ver["server"],
                    ver["cur"], ver["new"], int(ver["key"]), 
                    int(ver["key"]), int(ver["size"]))
            except Exception, e:
                error.append('dataid: %s, error[%s]: %s' % (id_, e.__class__.__name__, str(e)))

def register(zklist, dataids, agents):
    zkcli = KazooClient(hosts=zklist)
    zkcli.start()

    dataids = dataids.split(',')
    agents = agents.split(',')

    for id_ in dataids:
        for agent in agents:
            try:
                zkcli.create('/mem_sync/%s/agents/%s' % (id_, agent), makepath=True)
            except NodeExistsError, e:
                pass
            except Exception, e:
                print >> sys.stderr, str(e)
    zkcli.stop()


def show(zklist, dataids, agents):
    zkcli = KazooClient(hosts=zklist)
    zkcli.start()
    data_dict = {}

    ids_all = zkcli.get_children('/mem_sync')
    ids = dataids.split(',') if dataids else ids_all

    agents = agents.split(',') if agents else None

    for id_ in ids:
        data_dict[id_] = {}
        if id_ in ids_all:
            # agents we are interesting(monitoring)
            try:
                agents_to_mon = agents if agents \
                    else zkcli.get_children('/mem_sync/%s/agents' % id_)

                for agent in agents_to_mon:
                    data_dict[id_][agent] = {}
                    try:
                        value = zkcli.get('/mem_sync/%s/agents/%s' % (id_, agent))[0]
                        new_ver = zkcli.get('/mem_sync/%s/config/version' % (id_,))[0]
                        value = json.loads(value)
                        shm_key = zkcli.get('/mem_sync/%s/%s/shm_key' % \
                                (id_, value["version"]))[0]
                        shm_size = zkcli.get('/mem_sync/%s/%s/shm_size' % \
                                (id_, value["version"]))[0]
                        addr = zkcli.get('/mem_sync/%s/%s/data_server_addr' % \
                                (id_, value["version"]))[0].split(':')[0]

                        data_dict[id_][agent]["server"] = addr
                        data_dict[id_][agent]["cur"] = value["version"]  
                        data_dict[id_][agent]["new"] = new_ver
                        data_dict[id_][agent]["key"] = shm_key
                        data_dict[id_][agent]["size"] = shm_size
                    except Exception, e:
                        error.append('dataid: %s, error[%s]: %s' % \
                                (id_, e.__class__.__name__, str(e)))
            except Exception, e:
                error.append('dataid: %s, error[%s]: %s' % \
                        (id_, e.__class__.__name__, str(e)))

    zkcli.stop()
    format_data(data_dict)
    print ''
    for e in error: print e

if __name__ == '__main__':
    dataids = None
    agents = None
    server = None
    zklist = None
    command = 'show'

    opts, args = getopt.getopt(sys.argv[1:], "", 
        ["dataids=", "agents=", "zklist=", "help"])

    if not opts or len(args) > 1:
        usage()
    
    for k, v in opts:
        if k in ('--dataids', ):
            dataids = v
        elif k in ('--agents', ):
            agents = v
        elif k in ('--zklist', ):
            zklist = v
        elif k in ('--help', ):
            usage()

    if len(args) == 1:
        command = args[0]

    if command == 'show':
        show(zklist, dataids=dataids, agents=agents)
    elif command == 'register':
        register(zklist, dataids=dataids, agents=agents)
    else:
        usage()



