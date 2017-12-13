import logging
import homeassistant.loader as loader
import json
import time

from os import path
from homeassistant.helpers.event import track_time_interval
from homeassistant.helpers.entity import Entity
from homeassistant.util.json import load_json, save_json

DOMAIN = 'cat_feeder'
DEPENDENCIES = ['mqtt']
STORE_FILE = '.cat_feeder'
LOGGER = logging.getLogger(__name__)
# https://github.com/home-assistant/home-assistant-polymer/blob/ef36a31ac030b44ad271739a7c0e15946299cd8b/src/state-summary/state-card-scene.html

CONF_TRIGGER_TOPIC = 'trigger_topic'
CONF_MAX_PORTIONS = 'max_portions'
CONF_DAILY_PORTIONS = 'daily_portions'
CONF_STATE_TOPIC = 'state_topic'

DEFAULT_TRIGGER_TOPIC = 'home-assistant/cat_feeder'
DEFAULT_STATE_TOPIC = 'home-assistant/cat_feeder/set'

TODAY_PORTIONS_ENTITY_ID = '{}.today_portions'.format(DOMAIN)
TOTAL_PORTIONS_ENTITY_ID = '{}.portions_left'.format(DOMAIN)

ATTRIBUTES = {
  'unit_of_measurement': 'portions',
  'icon': 'mdi:food'
}

def store_path(hass):
  return hass.config.path(STORE_FILE)

class CatFeederManager:
  def __init__(self, hass, config, mqtt):
    self.data = {}
    self.hass = hass
    self.max_portions = config[DOMAIN].get(CONF_MAX_PORTIONS, 100)
    self.daily_portions = config[DOMAIN].get(CONF_DAILY_PORTIONS, 4)
    self.trigger_topic = config[DOMAIN].get(CONF_TRIGGER_TOPIC, DEFAULT_TRIGGER_TOPIC)
    self.state_topic = config[DOMAIN].get(CONF_STATE_TOPIC, DEFAULT_STATE_TOPIC)
    self.mqtt = mqtt
    
    mqtt.subscribe(hass, self.state_topic, self.state_message_received)
    self.load_data()
    self.save_data()

  def state_message_received(self, topic, payload, qos):
    LOGGER.debug("Received state at: {}".format(topic))
    self.reload_data()

  def feed(self, call):
    LOGGER.info("Sending feeding request")
    self.mqtt.publish(self.hass, self.trigger_topic, '')
    total = int(self.data['left'])
    if total > 0:
      self.data['left'] -= 1
      self.data['today'] += 1
    self.save_data()

  def save_data(self):
    if 'today' not in self.data:
      self.data['today'] = 0
    if 'left' not in self.data:
      self.data['left'] = self.max_portions

    if 'time' not in self.data or time.localtime().tm_mday == time.localtime(self.data['time']):
      self.data['today'] = 0
    self.data['time'] = time.time()
    save_json(store_path(self.hass), self.data)
    self.reload_data()

  def load_data(self):
    self.data = load_json(store_path(self.hass))

  def reload_data(self):
    self.load_data()

    LOGGER.error("Loaded: {} from {}".format(self.data, store_path(self.hass)))
    self.hass.states.set(TODAY_PORTIONS_ENTITY_ID, self.data['today'], ATTRIBUTES)
    self.hass.states.set(TOTAL_PORTIONS_ENTITY_ID, self.data['left'], ATTRIBUTES)

def setup(hass, config):
  mqtt = loader.get_component('mqtt')
  cat_feeder_manager = CatFeederManager(hass, config, mqtt)
  hass.services.register(DOMAIN, 'feed', cat_feeder_manager.feed)
  return True
