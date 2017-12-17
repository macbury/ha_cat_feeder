from intent_handler import IntentHandler
import random

class FeedTheCat(IntentHandler):
  def name(self):
    return 'cat.feed'

  def call(self, data):
    responses = [
      "Giving food to the cat",
      "Feeding the cat",
      "Here you go",
      "On the power of gray skull",
      "Here, kity kity kity"
    ]

    self.app.call_service('cat_feeder/feed')
    return random.choice(responses)

class InfoAboutFeeding(IntentHandler):
  def name(self):
    return 'cat.info'

  def call(self, data):
    today = self.app.get_state('cat_feeder.today_portions')
    return "The cat did get today {} portions".format(today)

class ReFill(IntentHandler):
  def name(self):
    return 'cat.refill'

  def call(self, data):
    self.app.call_service('cat_feeder/refill')
    return "Refilled food dispenser for cat"
