from flask import Flask

app = Flask('wizard_api')

@app.route('/')
def hello_world():
    return('Hello, World!')

if __name__ == '__main__':
    app.run(port=9001, host="0.0.0.0")
