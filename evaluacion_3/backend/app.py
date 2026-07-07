"""
============================================================
PROYECTO IOT - EVALUACIÓN SUMATIVA 3 - DCSH01
BACKEND FLASK
------------------------------------------------------------
Responsabilidades de este archivo:
  1) Recibir por HTTP (POST) los datos que envía la tarjeta
     Receptor/Gateway cada 3 segundos.
  2) Guardar cada evento en Supabase (con fecha y hora).
  3) Mostrar un monitor tipo terminal (HTML puro, sin
     JavaScript) con el último estado y el historial reciente.
  4) Permitir activar/desactivar el LED azul (modo manual)
     directamente desde el panel web -> esto se guarda en
     memoria y la tarjeta ESP32 lo consulta periódicamente.

Todo el backend está escrito en Python puro (Flask), sin uso
de JavaScript, cumpliendo las restricciones de la pauta.
============================================================
"""

import os
import json
from datetime import datetime

from flask import Flask, request, redirect, render_template
import requests
from dotenv import load_dotenv

# ------------------------------------------------------------
# Configuración inicial
# ------------------------------------------------------------
load_dotenv()  # Carga las variables definidas en el archivo .env

SUPABASE_URL = os.getenv("SUPABASE_URL")
SUPABASE_KEY = os.getenv("SUPABASE_KEY")
SUPABASE_TABLE = "eventos"

app = Flask(__name__)

# ------------------------------------------------------------
# Estado en memoria (para no tener que consultar Supabase en
# cada segundo solo para pintar el panel)
# ------------------------------------------------------------
estado_actual = {
    "pot_valor": None,
    "pot_porcentaje": None,
    "movimiento": False,
    "led_alerta": False,
    "ultima_actualizacion": None,
}

comando_manual = {
    "led_manual": False,
}


# ============================================================
# FUNCIONES DE ACCESO A SUPABASE (vía API REST)
# ============================================================

def guardar_en_supabase(registro: dict) -> None:
    """Inserta un nuevo evento en la tabla 'eventos' de Supabase."""
    url = f"{SUPABASE_URL}/rest/v1/{SUPABASE_TABLE}"
    headers = {
        "apikey": SUPABASE_KEY,
        "Authorization": f"Bearer {SUPABASE_KEY}",
        "Content-Type": "application/json",
        "Prefer": "return=minimal",
    }
    try:
        respuesta = requests.post(url, headers=headers, data=json.dumps(registro), timeout=5)
        if respuesta.status_code not in (200, 201, 204):
            print(f"[Supabase] Respuesta inesperada ({respuesta.status_code}): {respuesta.text}")
    except requests.exceptions.RequestException as error:
        print(f"[Supabase] Error al guardar: {error}")


def obtener_historial(limite: int = 15) -> list:
    """Obtiene los últimos N eventos guardados en Supabase, más recientes primero."""
    url = (
        f"{SUPABASE_URL}/rest/v1/{SUPABASE_TABLE}"
        f"?select=*&order=created_at.desc&limit={limite}"
    )
    headers = {
        "apikey": SUPABASE_KEY,
        "Authorization": f"Bearer {SUPABASE_KEY}",
    }
    try:
        respuesta = requests.get(url, headers=headers, timeout=5)
        if respuesta.status_code == 200:
            return respuesta.json()
        print(f"[Supabase] Respuesta inesperada ({respuesta.status_code}): {respuesta.text}")
    except requests.exceptions.RequestException as error:
        print(f"[Supabase] Error al leer historial: {error}")
    return []


# ============================================================
# ENDPOINTS PARA LA TARJETA ESP32 (RECEPTOR/GATEWAY)
# ============================================================

@app.route("/api/datos", methods=["POST"])
def recibir_datos():
    """Recibe el JSON enviado por el ESP32 cada 3 segundos y lo persiste."""
    cuerpo = request.get_json(force=True, silent=True) or {}

    pot_valor = cuerpo.get("pot_valor")
    pot_porcentaje = cuerpo.get("pot_porcentaje")
    movimiento = bool(cuerpo.get("movimiento", False))
    led_alerta = bool(cuerpo.get("led_alerta", False))

    # Actualiza el estado en memoria (para el panel)
    estado_actual["pot_valor"] = pot_valor
    estado_actual["pot_porcentaje"] = pot_porcentaje
    estado_actual["movimiento"] = movimiento
    estado_actual["led_alerta"] = led_alerta
    estado_actual["ultima_actualizacion"] = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    # Guarda el evento permanente en Supabase
    registro = {
        "pot_valor": pot_valor,
        "pot_porcentaje": pot_porcentaje,
        "movimiento": movimiento,
        "led_alerta": led_alerta,
        "comando_manual": comando_manual["led_manual"],
    }
    guardar_en_supabase(registro)

    return {"ok": True}, 200


@app.route("/api/comando", methods=["GET"])
def enviar_comando():
    """El ESP32 consulta este endpoint para saber si debe encender el LED manual."""
    return comando_manual, 200


# ============================================================
# ENDPOINTS PARA EL PANEL WEB (usuario humano)
# ============================================================

@app.route("/comando", methods=["POST"])
def actualizar_comando():
    """Botón del panel: activa o desactiva el modo manual (LED azul)."""
    accion = request.form.get("accion")
    comando_manual["led_manual"] = (accion == "activar")
    return redirect("/")


@app.route("/", methods=["GET"])
def dashboard():
    """Monitor principal, estilo terminal, con el último estado e historial."""
    historial = obtener_historial(15)
    return render_template(
        "dashboard.html",
        estado=estado_actual,
        comando=comando_manual,
        historial=historial,
    )


# ============================================================
# PUNTO DE ENTRADA
# ============================================================
if __name__ == "__main__":
    # host="0.0.0.0" permite que el ESP32 (en la misma red WiFi) alcance este servidor
    app.run(host="0.0.0.0", port=5000, debug=True)
