import paho.mqtt.client as mqtt
import json
import from_node
import log
import time

BROKER_ADDR = "127.0.0.1"
configs = dict()
nodes = dict()
mqtt_queue = []
MAX_MSG_QUEUE = 100
MQTT_PUB_INTERVAL = 5 

#---------------------------------

def add_in_queue(topic, msg):

    if len(mqtt_queue) > MAX_MSG_QUEUE:
        return
    else:
        mqtt_queue.append({'topic': topic, 'msg': msg})


def get_from_queue():

    if len(mqtt_queue) == 0:
        return -1
    else:
        return mqtt_queue.pop(0)

#--------------------------------

def mqtt_reset_config(land_id, node_id):
    index = "NODE/" + str(land_id) + "/" + str(node_id)
    if index not in configs:
        return
    configs[index].pop('irr_status')
    configs[index].pop('timer_status')

def mqtt_status(doc):
    index = "NODE/" + str(doc['body']['land_id']) + "/" + str(doc['body']['node_id'])
    if not (index in configs):
        configs[index] = dict()

    configs[index][doc['cmd']] = doc['body']
    if ('irr_status' in configs[index] and
        'timer_status' in configs[index]
        ):

        land_id = configs[index]['irr_status']['land_id']
        node_id = configs[index]['irr_status']['node_id']
        enabled = configs[index]['irr_status']['enabled']
        irr_limit = configs[index]['irr_status']['irr_limit']
        irr_duration = configs[index]['irr_status']['irr_duration']
        mst_timer = configs[index]['timer_status']['mst_timer']
        ph_timer = configs[index]['timer_status']['ph_timer']
        light_timer = configs[index]['timer_status']['light_timer']
        tmp_timer =  configs[index]['timer_status']['tmp_timer']

        msg = { 'cmd': 'status', 'body': { 'land_id': land_id, 'node_id': node_id, 'irr_config': { 'enabled': enabled, 'irr_limit': irr_limit, 'irr_duration': irr_duration }, 'mst_timer': mst_timer, 'ph_timer': ph_timer, 'light_timer': light_timer, 'tmp_timer': tmp_timer } } 
        from_node.status("MQTT", "", msg)

#-------------------------


def add_node(land_id, node_id):
    index = f"NODE/{land_id}/{node_id}"
    if index not in nodes:
        nodes[index] = True

def delete_node(land_id, node_id):
    index = f"NODE/{land_id}/{node_id}"
    if index in nodes:
        nodes.pop(index)

def get_nodes():
    list_of_nodes = []
    for key in nodes.keys():
        index = (key.replace("NODE/", "")).split('/')
        list_of_nodes.append({'land_id': index[0], 'node_id': index[1], 'protocol': 'MQTT', 'addr': 'null'})
    return list_of_nodes


def check_node(land_id, node_id):
    index = f"NODE/{land_id}/{node_id}"
    if index in nodes:
        return True
    else:
        return False

def show_mqtt_nodes():

    log.log_console("+---------------------------------+")
    log.log_console("|      CONFIGURED MQTT NODES      |")
    log.log_console("+---------------------------------+")
    for key in nodes.keys():
        log.log_console(f"  index: {key}")
    log.log_console("-----------------------------------")

#--------------------------

# The callback for when the client receives a CONNACK response from the broker.
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    client.subscribe("SERVER")

#------------------------

def parse_json(payload):
    json_doc = json.loads(payload[1:len(payload)-1])
    return json_doc

#------------------------

# The callback for when a PUBLISH message is received from the broker.
def on_message(client, userdata, msg):
    payload = (str(msg.payload))[1:]
    doc = parse_json(payload)
    log.log_receive(doc, doc['body']['land_id'], doc['body']['node_id'])
    if doc['cmd'] == 'config_rqst':
        from_node.config_request("MQTT", "null", doc)
    elif doc['cmd'] == 'irr_status' or doc['cmd'] == 'timer_status':
        mqtt_status(doc)
    elif doc['cmd'] == 'irrigation':
        from_node.irrigation(doc)
    elif doc['cmd'] == 'moisture':
        from_node.moisture(doc)
    elif doc['cmd'] == 'ph':
        from_node.ph(doc)
    elif doc['cmd'] == 'light':
        from_node.light(doc)
    elif doc['cmd'] == 'tmp':
        from_node.tmp(doc)
    elif doc['cmd'] == 'is_alive_ack':
        from_node.is_alive_ack(doc, "MQTT")

client = mqtt.Client()

#------------------------

def mqtt_init():
    global client 
    client.connect(BROKER_ADDR, 1883, 60)

#------------------------

def mqtt_subscribe():
    global client
    client.on_connect = on_connect
    client.on_message = on_message
    client.loop_forever()
#------------------------

def mqtt_publish(topic, msg):
    add_in_queue(topic, msg)


def mqtt_publisher_loop():
    
    global client

    end_timer =  MQTT_PUB_INTERVAL
    start_timer = time.time()
    while True:
        
        #check if nodes are online
        if (time.time() - start_timer) >= end_timer:
            msg_to_publish = get_from_queue()
            if msg_to_publish != -1:
                client.publish(msg_to_publish['topic'], payload=msg_to_publish['msg'])
            start_timer = time.time()

        time.sleep(0.1)