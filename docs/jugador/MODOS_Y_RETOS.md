# Modos del HUB y retos (guía para jugar)

*English version below.*

## Dónde están
En el menú principal, baja (o sube) más allá de la última fila: aparece la fila **HUB**. Dentro:

| Entrada | Qué es |
|---|---|
| Tag Team (beta) | Equipos de 2 o 3 luchadores. SELECT en combate para relevar al compañero. |
| Online (beta) | Duelo contra otra persona por internet (desde el menú de Duelo). |
| Combate rápido | Un combate al momento contra el ordenador, luchadores y escenario al azar. |
| Torre | Pisos seguidos contra rivales cada vez más fuertes. |
| Supervivencia | Oleadas seguidas: la vida que te queda pasa a la siguiente y recuperas un poco entre medias. |
| Kumite | Jefes con mucha vida, uno tras otro. Tu vida también se arrastra. |
| Ranking / Laboratorio | Aún en gris: llegarán en una versión próxima. |

En Torre, Supervivencia y Kumite **juegas con el último luchador que usaste en el Duelo**. Cada vez que ganas,
el juego pasa solo al combate siguiente. Si pierdes, el reto termina y se apunta hasta dónde llegaste. Si eliges
"Return to Main Menu" entre combates, te retiras con lo ganado.

Tus resultados se guardan en `user_data/dbz3/retos.csv` (cada reto) y `user_data/dbz3/combates.csv` (cada combate,
de ahí sale el ranking). Nada de esto toca tu partida guardada.

## Retos predefinidos
Cada modo usa un reto. Sin hacer nada tienes el de serie. La carpeta `mod center hd` del Mod Kit trae un catálogo listo para jugar:

| Modo | Reto | Combates |
|---|---|---|
| Torre | Saga Saiyan: Saibaman, Raditz, Nappa y Vegeta | 4 |
| Torre | Namek: Recoome, Ginyu, Vegeta y Freezer | 4 |
| Torre | Androides: Dr. Gero, 18, 17, 16, Cell Jr. y Cell | 6 |
| Torre | Buu: Dabura y las tres formas de Buu | 4 |
| Torre | Leyendas: Bardock, Cooler, Broly y Omega Shenron | 4 |
| Torre | Guerreros Z: de Yamcha a Goku | 7 |
| Supervivencia | **Misión 100**: 100 oleadas con todo el plantel, empiezas con 7 barras | 100 |
| Supervivencia | Invasión de Saibaman: 12 oleadas sin recuperar vida | 12 |
| Supervivencia | Una sola barra: 8 rivales con una barra para todo el reto | 8 |
| Kumite | Villanos: Freezer, Cell, Cooler, Kid Buu, Broly y Omega Shenron | 6 |
| Kumite | Saiyan: Raditz, Nappa, Vegeta, Bardock y Broly | 5 |
| Kumite | Exprés: Freezer, Cell y Kid Buu | 3 |

Cómo poner uno (con el juego cerrado), desde la carpeta `mod center hd` del Mod Kit:

```
python retos_predefinidos.py lista --juego "C:\Juegos\DBZ3HD"
python retos_predefinidos.py instalar nativo_torre_namek --juego "C:\Juegos\DBZ3HD"
python retos_predefinidos.py semana --juego "C:\Juegos\DBZ3HD"            (la torre de esta semana)
python retos_predefinidos.py quitar torre --juego "C:\Juegos\DBZ3HD"      (vuelve el de serie)
```

- **Torre de la semana**: cada semana toca una torre distinta del catálogo, la misma toda la semana. Reiniciar
  no la cambia, y un reto empezado no cambia a mitad.
- Nunca se borra nada: el reto que había antes se guarda en `user_data/dbz3/modos/_respaldo/`.
- Puedes crear los tuyos: `python modos_reto.py nativo torre --rivales 42,18,19,7 --vida-rival 2000:400 --juego ...`.

---

# HUB modes and challenges (player guide)

Go past the last row of the main menu to reach the **HUB** row. It holds Tag Team (beta), Online (beta), Quick
Battle, Tower, Survival and Kumite (Ranking and Laboratory are greyed out for now).

In Tower, Survival and Kumite you play with **the last fighter you used in Duel**. Each win moves on to the next
fight automatically; a loss ends the challenge and your progress is recorded. In Survival and Kumite your remaining
health carries over to the next fight, plus a small recovery. Results go to `user_data/dbz3/retos.csv` and
`user_data/dbz3/combates.csv` (used by the ranking); your save file is never touched.

**Ready-made challenges** (Mod Kit, `mod center hd` folder, game closed):
`python retos_predefinidos.py lista|instalar <name>|semana|quitar <mode> --juego "<game folder>"`.
The catalog has six towers (Saiyan Saga, Namek, Androids, Buu, Legends, Z Fighters), three survivals
(**Mission 100** with 100 waves, Saibaman Invasion, One Bar Only) and three kumites (Villains, Saiyan, Express).
`semana` installs the tower of the week (same all week, a different one next week). Whatever was installed before
is moved to `user_data/dbz3/modos/_respaldo/`, never deleted.
