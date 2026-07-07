from flask import Flask, render_template
import socket
import math

app = Flask(__name__)

HOST = "192.168.100.39"     ## ip del servidor
PORT = 12345                ## Puerto del servidor

RANGO = 10.0          # rango +/- esperado de los valores del sensor (ajustar si es necesario)
TAMANO_GRILLA = 8     # cuadrícula 8x8 (la pauta exige mínimo 4x4)
NOMBRE_ALUMNO = "Esteban Rivera Zurita" 


def leer_sensor():
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(2)  # evita que la petición se cuelgue si el sensor no responde
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


def indice_grilla(valor, invertir=False):
    """Convierte un valor del sensor en un índice de fila/columna dentro de la grilla."""
    v = max(-RANGO, min(RANGO, valor))
    if invertir:
        v = -v
    idx = round((v + RANGO) / (2 * RANGO) * (TAMANO_GRILLA - 1))
    return max(0, min(TAMANO_GRILLA - 1, idx))


def porcentaje(valor):
    """Convierte un valor del sensor (-RANGO..+RANGO) en un porcentaje 0..100."""
    v = max(-RANGO, min(RANGO, valor))
    return round((v + RANGO) / (2 * RANGO) * 100, 1)


def color_por_signo(valor):
    return "var(--rojo)" if valor < 0 else "var(--verde)"


def color_por_nivel(pct):
    if pct < 35:
        return "var(--verde)"
    elif pct < 70:
        return "var(--ambar)"
    return "var(--rojo)"


@app.route('/')
def index():
    ejeY, ejeX = leer_sensor()  # órden ejes.
    fila_activa = indice_grilla(ejeY, invertir=True)
    columna_activa = indice_grilla(ejeX)
    return render_template(
        'index.html',
        ejeX=ejeX, ejeY=ejeY,
        tamano_grilla=TAMANO_GRILLA,
        fila_activa=fila_activa,
        columna_activa=columna_activa,
        nombre_alumno=NOMBRE_ALUMNO
    )


@app.route('/termometros')
def termometros():
    ejeY, ejeX = leer_sensor()  # órden ejes.
    pct_x = porcentaje(ejeX)
    pct_y = porcentaje(ejeY)
    return render_template(
        'termometros.html',
        ejeX=ejeX, ejeY=ejeY,
        pct_x=pct_x, pct_y=pct_y,
        color_x=color_por_signo(ejeX),
        color_y=color_por_signo(ejeY),
        nombre_alumno=NOMBRE_ALUMNO
    )


@app.route('/arma')
def arma():
    ejeY, ejeX = leer_sensor()  # órden ejes.
    magnitud = math.sqrt(ejeX ** 2 + ejeY ** 2)
    pct_mag = min(100, round(magnitud / (RANGO * 1.2) * 100))
    angulo_aguja = -80 + (pct_mag / 100) * 160
    return render_template(
        'arma.html',
        ejeX=ejeX, ejeY=ejeY,
        pct_mag=pct_mag,
        angulo_aguja=angulo_aguja,
        color_nucleo=color_por_nivel(pct_mag),
        nombre_alumno=NOMBRE_ALUMNO
    )


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
