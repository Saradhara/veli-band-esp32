from flask import Flask, jsonify, request

app = Flask(__name__)


@app.route("/")
def hello():
    return "Welcome to QUAL5 IOT world !!!!!"


@app.route("/trace", methods=["POST"])
def update_trace():
    data = request.data
    traces = data.decode('utf-8').split('@@@')
    traces = [trace for trace in traces if trace != '']
    print("Total {} traces received !!! ".format(len(traces)))
    for trace in traces:
        uuid, distance, risk = trace.split("#")
        print('uuid       : {}'.format(uuid))
#        print('distance   : {}'.format(distance))
        print('COVID risk : {}'.format(risk))
    return jsonify({"data_status": "received"})


if __name__ == "__main__":
    app.run(debug=True, host='0.0.0.0', port=8888)
