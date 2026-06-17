from flask import Flask, render_template, jsonify
import socket

app = Flask(__name__)

HOST = "10.178.118.219"     ## ip del servidor
PORT = 12345                ## Puerto del servidor


def leer_sensor():
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(2)  # evita que la petición se cuelgue si el servidor no responde
            s.connect((HOST, PORT))
            data = s.recv(1024).decode().strip()
            print("RECIBIDO:", data)
            ejeX, ejeY = data.split(',')
            ejeX = float(ejeX.replace('X:', ''))
            ejeY = float(ejeY.replace('Y:', ''))
            return ejeX, ejeY
    except Exception as e:
        print("ERROR:", e)
        return 0, 0


@app.route('/')
def index():
    ejeY, ejeX = leer_sensor()  # órden ejes.
    return render_template(
        'index.html',
        ejeX=ejeX,
        ejeY=ejeY
    )


@app.route('/arma')
def arma():
    return render_template('arma.html')


@app.route('/api/sensor')
def api_sensor():
    """Endpoint que consulta el sensor en tiempo real para el polling AJAX del HUD."""
    ejeY, ejeX = leer_sensor()  # mismo órden de ejes que en index()
    return jsonify(ejeX=ejeX, ejeY=ejeY)


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True, threaded=True)
