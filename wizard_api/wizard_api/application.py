from flask import Flask
import os

SHARED_SECRET = os.getenv('SECRET_KEY', 'abcdefg')

app = Flask("wizard_api")

@app.route("/")
def hello_world():
    return("Hello, World!")

@app.route("/new")
def new_route():
    return("This is a brand new route!")

if __name__ == "__main__":
    app.run(port=9001, host="0.0.0.0", use_reloader=True, debug=True)

# There's a few things the backend has to do.
# Authentication.
# Request to play a game.
# - Return a connect token.
# Client uses the connect token to establish connection to the game server.
# Once the connection is established the server and client can exchange encrypted and signed UDP packets.


# There is a private key that must be shared between the web backend and the server instances.

# One day, there will be session management and hosting and lists of games to join and matchmaking and wow.
# But TODAY. There is just 1 server and anybody can join it.
