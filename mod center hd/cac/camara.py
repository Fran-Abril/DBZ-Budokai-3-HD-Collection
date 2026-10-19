#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""camara.py - Camara propia del creador de personaje (orbita / zoom / giro auto).

Frente 02 (studies 6). La camara orbita alrededor de un punto del modelo (por
defecto la cabeza o el centro del cuerpo), con giro automatico opcional ("turntable")
y control manual (raton/palancas en el juego; aqui, parametros). Devuelve la matriz
de vista 3x3 que usa `model_render.render` (x derecha, y arriba, z hacia la camara).

Reutilizable por `pantalla.py` (vista previa sin GPU) y por `src/cac/` (vista en el
juego). La parte "en juego" (dibujar el modelo con la camara del creador) es la
propuesta de `src/cac/cac.cpp`; esta clase fija el ESTADO (yaw/pitch/dist) y su spec.
"""
import math


class Camara:
    """Camara orbital. Angulos en grados, distancia en unidades del modelo."""

    def __init__(self, yaw=4.0, pitch=8.0, dist=2.6, target=(0.0, 0.0, 0.0),
                 spin=18.0, yaw_min=-80.0, yaw_max=80.0, pitch_min=-25.0, pitch_max=45.0,
                 dist_min=0.8, dist_max=6.0):
        self.yaw, self.pitch, self.dist = yaw, pitch, dist
        self.target = target
        self.spin = spin                   # grados/segundo del giro automatico (0 = quieta)
        self.spin_on = True
        self.yaw_min, self.yaw_max = yaw_min, yaw_max
        self.pitch_min, self.pitch_max = pitch_min, pitch_max
        self.dist_min, self.dist_max = dist_min, dist_max

    # -- controles -----------------------------------------------------------
    def orbitar(self, dyaw, dpitch):
        self.spin_on = False               # al mover a mano, se para el giro
        self.yaw = _clamp(self.yaw + dyaw, self.yaw_min, self.yaw_max)
        self.pitch = _clamp(self.pitch + dpitch, self.pitch_min, self.pitch_max)

    def acercar(self, factor):
        self.dist = _clamp(self.dist * factor, self.dist_min, self.dist_max)

    def girar(self, grados):
        self.yaw = _clamp(self.yaw + grados, self.yaw_min, self.yaw_max)

    def actualizar(self, dt):
        if self.spin_on and self.spin:
            self.yaw = _clamp(self.yaw + self.spin * dt, self.yaw_min, self.yaw_max)
            if self.yaw >= self.yaw_max or self.yaw <= self.yaw_min:
                self.spin = -self.spin      # rebote suave

    # -- salida --------------------------------------------------------------
    def view(self):
        """Matriz de vista 3x3 (rot_x(pitch) * rot_y(yaw)), como model_render."""
        return _mmul(_rot_x(self.pitch), _rot_y(self.yaw))

    def spec(self):
        return {"yaw": self.yaw, "pitch": self.pitch, "dist": self.dist,
                "target": list(self.target), "spin": self.spin, "spin_on": self.spin_on,
                "limites": {"yaw": [self.yaw_min, self.yaw_max],
                            "pitch": [self.pitch_min, self.pitch_max],
                            "dist": [self.dist_min, self.dist_max]}}


def _clamp(v, lo, hi):
    return lo if v < lo else (hi if v > hi else v)


def _mmul(a, b):
    return [[sum(a[i][k] * b[k][j] for k in range(3)) for j in range(3)] for i in range(3)]


def _rot_y(deg):
    a = math.radians(deg)
    return [[math.cos(a), 0.0, math.sin(a)], [0.0, 1.0, 0.0], [-math.sin(a), 0.0, math.cos(a)]]


def _rot_x(deg):
    a = math.radians(deg)
    return [[1.0, 0.0, 0.0], [0.0, math.cos(a), -math.sin(a)], [0.0, math.sin(a), math.cos(a)]]


# Presets de camara del creador (el usuario pidio "camara propia" con vistas claras).
PRESETS = {
    "frente": Camara(yaw=0.0, pitch=6.0, dist=2.2, spin=0),
    "tres_cuartos": Camara(yaw=28.0, pitch=10.0, dist=2.4, spin=18),
    "retrato": Camara(yaw=6.0, pitch=6.0, dist=1.1, target=(0.0, 1.35, 0.0), spin=8),
    "cuerpo_entero": Camara(yaw=20.0, pitch=4.0, dist=4.2, spin=12),
    "giro_auto": Camara(yaw=0.0, pitch=8.0, dist=2.6, spin=35),
}


def preset(nombre):
    """Copia de un preset (para no compartir estado)."""
    import copy
    return copy.deepcopy(PRESETS[nombre])


if __name__ == "__main__":
    c = preset("tres_cuartos")
    print("spec:", c.spec())
    c.actualizar(0.5)
    print("tras 0.5 s de giro:", round(c.yaw, 2), c.spec()["spin_on"])
