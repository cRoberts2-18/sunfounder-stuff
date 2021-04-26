#! /usr/bin/python3
import pandas as pd
import numpy
import firebase_admin
from firebase_admin import credentials, firestore

cred = credentials.Certificate("1234/pisensor-36470-firebase-adminsdk-2wl8o-986e0f0216.json")
firebase_admin.initialize_app(cred, 
{
"databaseURL" : "https://PiSensor.firebaseio.com/"
})

db = firestore.client()
doc_ref = db.collection(u"Readings")

file = pd.read_csv("Readings.csv")
temp = file.to_dict(orient="records")
list(map(lambda x: doc_ref.add(x), temp))

