import getopt
import sys
import socket
import sys
import log
import json
import to_node
import from_node
from persistence import update_mysql_db
from coapthon.server.coap import CoAP
from coapthon.resources.resource import Resource
from coapthon.client.helperclient import HelperClient
from coapthon.utils import parse_uri

nodes = dict()
my_ip = "fd00::1"
port = 5683
SEND_TIMEOUT = 3.0
configs = dict()

#----------------------

def extract_addr(request):
    addr = str(request)
#    addr = addr.split(",")
#    addr = addr[0].split("(")
#    addr = addr[1].split(",")
#    addr = addr[0].replace("\'", "")
    addr = addr.split("(")
    addr = addr[1].split(")")
    addr = addr[0].split(',')
    addr = addr[0].replace("\'","")
    return addr

#----------------------

def new_client(addr):
    host = addr
    client = HelperClient(server=(host, port))
    return client

#----------------------

def client_observe(land_id, node_id, path):
    index = "NODE/" + str(land_id) + "/" + str(node_id)
    client = nodes[index]['host']
    client.observe(path, client_callback)

#----------------------

def add_nodes(land_id, node_id, addr):
    index = "NODE/" + str(land_id) + "/" + str(node_id)
    if (index in nodes) and nodes[index]['addr'] == addr:
        return
    else:
        nodes[index] = dict()
        nodes[index]['addr'] = addr
        nodes[index]['host'] = new_client(addr)

#----------------------

def show_coap_nodes():

    log.log_info("List of known nodes")
    keys = [ key for key, val in nodes.items()]
    for key in keys:
        print(f"index: {key} addr: {nodes[key]['addr']}")
    print("-----------------------")
        
#----------------------

def delete_node(land_id, node_id):
    index = "NODE/" + str(land_id) + "/" + str(node_id)
    log.log_info(f"deleting node {index} from cache")
    if index in nodes:
        nodes[index]['host'].stop()
        nodes.pop(index)

#----------------------

def reset_config(land_id, node_id):
    index = "NODE/" + str(land_id) + "/" + str(node_id)
    if index not in configs:
        return
    configs[index].pop('irr-status')
    configs[index].pop('mst-status')
    configs[index].pop('ph-status')
    configs[index].pop('light-status')
    configs[index].pop('tmp-status')

def coapStatus(land_id, node_id, doc):
    index = "NODE/" + str(land_id) + "/" + str(node_id)
    if not (index in configs):
        configs[index] = dict()

    configs[index][doc['cmd']] = doc
    if ('config-status' in configs[index] and
        'irr-status' in configs[index] and
        'mst-status' in configs[index] and
        'ph-status' in configs[index] and
        'light-status' in configs[index] and
        'tmp-status'in configs[index] 
        ):

        irr_config = configs[index]['irr-status']['body']
        mst_timer = configs[index]['mst-status']['timer']
        ph_timer = configs[index]['ph-status']['timer']
        light_timer = configs[index]['light-status']['timer']
        tmp_timer = configs[index]['tmp-status']['timer']

        msg = { 'cmd': 'status', 'body': { 'land_id': land_id, 'node_id': node_id, 'irr_config': { 'enabled': irr_config['enabled'], 'irr_limit': irr_config['irr_limit'], 'irr_duration': irr_config['irr_duration'] }, 'mst_timer': mst_timer, 'ph_timer': ph_timer, 'light_timer': light_timer, 'tmp_timer': tmp_timer } } 
        from_node.status("COAP", nodes[index]['addr'], msg)

# --------------- CLIENT---------------- #

def send_msg(land_id, node_id, path, mode, msg):
    index = "NODE/" + str(land_id) + "/" + str(node_id)
    if not (index in nodes):
        log.log_err(f"{index} address unknown")
        return False
    
    client = new_client(nodes[index]['addr'])
    response = None
    if mode == "GET":
        response = client.get(path, timeout=SEND_TIMEOUT)
    elif mode == "PUT":
        response = client.put(path, msg, timeout=SEND_TIMEOUT)
    
    client.stop()
    return client_callback(response)
#    if response == None or response.payload == "" or response.payload == None:
#        log.log_err(f"node {index} doesn't respond [{path}]")
#        return False
#
#    log.log_info(f"received (coap) {response.payload}")
#
#    doc = json.loads(response.payload)
#    if doc['cmd'].find("status") >= 0:
#        if doc['cmd'] == 'config-status':
#            if int(doc['body']['land_id']) != int(land_id) or int(doc['body']['node_id']) != int(node_id):
#                log.log_err(f"node {index} address is changed")
#                update_mysql_db.update_address_in_configuration(land_id, node_id, "null", "null")
#                delete_node(land_id, node_id)
#                return False
#
#        coapStatus(land_id, node_id, doc)
#    elif doc['cmd'] == "irrigation":
#        msg = { 'cmd': doc['cmd'], 'body': { 'land_id': land_id, 'node_id': node_id, 'status': doc['status'] } }
#        from_node.irrigation(msg)
#    elif doc['cmd'] == "mst":
#        msg = { 'cmd':'moisture', 'body': { 'land_id': land_id, 'node_id': node_id, 'type': 'moisture', 'value': doc['value'] } }
#        from_node.moisture(msg)
#    elif doc['cmd'] == "ph":
#        msg = { 'cmd':'ph', 'body': { 'land_id': land_id, 'node_id': node_id, 'type': 'ph', 'value': doc['value'] } }
#        from_node.ph(msg)
#    elif doc['cmd'] == "light":
#        msg = { 'cmd':'light', 'body': { 'land_id': land_id, 'node_id': node_id, 'type': 'light', 'value': doc['value'] } }
#        from_node.light(msg)
#    elif doc['cmd'] == "tmp":
#        msg = { 'cmd':'tmp', 'body': { 'land_id': land_id, 'node_id': node_id, 'type': 'tmp', 'value': doc['value'] } }
#        from_node.tmp(msg)
#    elif doc['cmd'] == "is_alive_ack":
#        log.log_success(f"node {index} online")
#        msg = { 'cmd': doc['cmd'], 'body': { 'land_id': land_id, 'node_id': node_id } }
#        from_node.is_alive_ack(msg)
#
#    return True

#----------------------

def client_callback(response):

    if response == None:
        log.log_err("received None")
        return False
    addr = extract_addr(response)
    index = [ key for key in nodes.items() if key[1]['addr'] == addr][0][0]
    index = index.replace("NODE/","")
    index = index.split("/")
    land_id = index[0]
    node_id = index[1]

    if response.payload == None or response.payload == "":
        log.log_err(f"received None from ({land_id}, {node_id})")
        return False

    log.log_info(f"received {response.payload} from ({land_id}, {node_id})")
    doc = json.loads(response.payload)
    if doc['cmd'].find("status") >= 0:
        if doc['cmd'] == 'config-status':
            if int(doc['body']['land_id']) != int(land_id) or int(doc['body']['node_id']) != int(node_id):
                log.log_err(f"node {index} address is changed")
                update_mysql_db.update_address_in_configuration(land_id, node_id, "null", "null")
                delete_node(land_id, node_id)
                return False

        coapStatus(land_id, node_id, doc)
    elif doc['cmd'] == "irrigation":
        msg = { 'cmd': doc['cmd'], 'body': { 'land_id': land_id, 'node_id': node_id, 'status': doc['status'] } }
        from_node.irrigation(msg)
    elif doc['cmd'] == "mst":
        msg = { 'cmd':'moisture', 'body': { 'land_id': land_id, 'node_id': node_id, 'type': 'moisture', 'value': doc['value'] } }
        from_node.moisture(msg)
    elif doc['cmd'] == "ph":
        msg = { 'cmd':'ph', 'body': { 'land_id': land_id, 'node_id': node_id, 'type': 'ph', 'value': doc['value'] } }
        from_node.ph(msg)
    elif doc['cmd'] == "light":
        msg = { 'cmd':'light', 'body': { 'land_id': land_id, 'node_id': node_id, 'type': 'light', 'value': doc['value'] } }
        from_node.light(msg)
    elif doc['cmd'] == "tmp":
        msg = { 'cmd':'tmp', 'body': { 'land_id': land_id, 'node_id': node_id, 'type': 'tmp', 'value': doc['value'] } }
        from_node.tmp(msg)
    elif doc['cmd'] == "is_alive_ack":
        msg = { 'cmd': doc['cmd'], 'body': { 'land_id': land_id, 'node_id': node_id } }
        from_node.is_alive_ack(msg)

    return True



# ------------ SERVER ---------------- #



class ConfigurationRes(Resource):

    def __init__(self, name="Config Portal", coap_server=None):
        super(ConfigurationRes, self).__init__(name, coap_server, visible=True, observable=True, allow_children=True)

        self.payload = "Basic Resource"
        self.resource_type = "rt1"
        self.content_type = "text/plain"
        self.interface_type = "if1"

    def render_GET(self, request):
        msg = json.loads(request.payload)
        addr = extract_addr(request)
        to_print = "received " + str(msg) + " from " + addr
        log.log_info(to_print)
        self.payload = to_node.assign_config(msg['land_id'], msg['node_id'], "COAP", addr, False)
        add_nodes(msg['land_id'], msg['node_id'], addr)
        client_observe(msg['land_id'], msg['node_id'], "/irrigation")
        client_observe(msg['land_id'], msg['node_id'], "/sensor/mst")
        client_observe(msg['land_id'], msg['node_id'], "/sensor/ph")
        client_observe(msg['land_id'], msg['node_id'], "/sensor/light")
        client_observe(msg['land_id'], msg['node_id'], "/sensor/tmp")
        return self

#----------------------

class CoAPServer(CoAP):
    def __init__(self, host, port):
        CoAP.__init__(self, (host, port), False)
        self.add_resource("new_config", ConfigurationRes())

#----------------------

def listener():

    server = CoAPServer(my_ip, port)

    try:
        server.listen(10)
    except KeyboardInterrupt:
        print("Server Shutdown")
        server.close()
        print("Exiting...")