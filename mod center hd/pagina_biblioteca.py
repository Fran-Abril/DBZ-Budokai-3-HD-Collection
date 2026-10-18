#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""pagina_biblioteca.py - Pagina "Biblioteca de animaciones" del Mod Kit (clase BibliotecaPage).

Diseno en 3 columnas: (1) buscador + filtro por categoria + lista de clips; (2) vista previa de la pose
del clip en el fotograma elegido (render toon de model_render, pose estatica, sin reproduccion);
(3) receta: fuente = clip seleccionado, destino = accion a reemplazar, y 'Guardar receta' (.toml).

Integracion: copiar este fichero y biblioteca_anim.py a 'mod center hd/', registrar la pagina en
modkit_gui.py: ADV_TABS += [("biblioteca", "Biblioteca", "Library")] y
PAGE_CLASSES["biblioteca"] = BibliotecaPage (importada de pagina_biblioteca).
Solo lee datos del juego; no escribe en us/ ni en mods/.

  python pagina_biblioteca.py --selftest     prueba headless (ventana oculta) contra el us/ del proyecto
"""
import os
import queue
import sys
import threading
import tkinter as tk
from tkinter import filedialog, messagebox, ttk

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)
import biblioteca_anim as bib  # noqa: E402  (prepara sys.path con mod center hd/ y awo_tools/)
# Dentro del Kit (modkit_gui.py lanzado como __main__) se usan SUS objetos: importarlo otra vez crearia otra copia del
# modulo con las fuentes (F) sin iniciar y el idioma por defecto. Suelto (--selftest de esta pagina), el modulo normal.
_kit = sys.modules.get("__main__")
if not all(hasattr(_kit, n) for n in ("C", "F", "T", "Page", "init_fonts", "label", "setup_style")):
    import modkit_gui as _kit  # noqa: E402
C, F, T, Page, init_fonts, label, setup_style = (_kit.C, _kit.F, _kit.T, _kit.Page, _kit.init_fonts, _kit.label,
                                                 _kit.setup_style)

TODAS = "__todas__"


class BibliotecaPage(Page):
    key = "biblioteca"

    # ------------------------------------------------------------ construccion
    def build(self):
        self.filas, self.visibles, self.sel, self.destino = [], [], None, None
        self._cargado = False
        self._pend = None
        self._photo = None
        self._cola = queue.Queue()
        self.header(T("Biblioteca de animaciones", "Animation library"),
                    T("Busca cualquier clip de los personajes del juego, mira su pose y guarda una receta. "
                      "Solo lee: no cambia el juego ni tus mods.",
                      "Search any clip of the game's characters, look at its pose and save a recipe. "
                      "Read-only: it does not change the game or your mods."))

        bar = tk.Frame(self, bg=C["bg"])
        bar.pack(fill="x", padx=22, pady=(0, 6))
        self.var_q = tk.StringVar()
        self.var_q.trace_add("write", lambda *_: self._refrescar())
        tk.Entry(bar, textvariable=self.var_q, bg=C["card"], fg=C["text"], insertbackground=C["text"],
                 relief="flat", font=F["body"]).pack(side="left", fill="x", expand=True, ipady=5)
        self.cats = [TODAS] + list(bib.CATEGORIAS)
        self.nombres_cat = [T("Todas las categorías", "All categories")] + list(bib.CATEGORIAS)
        self.cb_cat = ttk.Combobox(bar, values=self.nombres_cat, state="readonly", width=20)
        self.cb_cat.current(0)
        self.cb_cat.pack(side="left", padx=(8, 0))
        self.cb_cat.bind("<<ComboboxSelected>>", lambda _e: self._refrescar())
        self.status = label(self, T("Cargando la biblioteca del juego...", "Loading the game library..."),
                            fg=C["dim"], wrap=900)
        self.status.pack(fill="x", padx=22, pady=(0, 6))

        body = tk.Frame(self, bg=C["bg"])
        body.pack(fill="both", expand=True, padx=22, pady=(0, 10))
        body.columnconfigure(0, weight=5, uniform="c")
        body.columnconfigure(1, weight=4, uniform="c")
        body.columnconfigure(2, weight=3, uniform="c")
        body.rowconfigure(0, weight=1)

        # (1) lista
        left = tk.Frame(body, bg=C["card"])
        left.grid(row=0, column=0, sticky="nsew", padx=(0, 10))
        cols = ("pers", "cod", "anim", "fr", "cat")
        self.tree = ttk.Treeview(left, columns=cols, show="headings", selectmode="browse", height=18)
        for c, t, w in (("pers", T("Personaje", "Character"), 150), ("cod", T("Código", "Code"), 62),
                        ("anim", T("Clip", "Clip"), 54), ("fr", T("Frames", "Frames"), 60),
                        ("cat", T("Categoría", "Category"), 100)):
            self.tree.heading(c, text=t)
            self.tree.column(c, width=w, anchor="w", stretch=(c == "pers"))
        sc = ttk.Scrollbar(left, orient="vertical", command=self.tree.yview)
        self.tree.configure(yscrollcommand=sc.set)
        self.tree.pack(side="left", fill="both", expand=True)
        sc.pack(side="right", fill="y")
        self.tree.bind("<<TreeviewSelect>>", lambda _e: self._al_seleccionar())

        # (2) vista previa
        mid = tk.Frame(body, bg=C["card"])
        mid.grid(row=0, column=1, sticky="nsew", padx=(0, 10))
        self.lbl_img = tk.Label(mid, bg=C["log_bg"], fg=C["dim"], font=F["body"],
                                text=T("Elige un clip de la lista", "Pick a clip from the list"))
        self.lbl_img.pack(fill="both", expand=True, padx=10, pady=10)
        self.sc_frame = tk.Scale(mid, from_=0, to=0, orient="horizontal", label=T("Fotograma", "Frame"),
                                 bg=C["card"], fg=C["text"], troughcolor=C["frame"], highlightthickness=0,
                                 command=lambda _v: self._programar_render())
        self.sc_frame.pack(fill="x", padx=10)
        self.sc_yaw = tk.Scale(mid, from_=0, to=360, orient="horizontal", label=T("Giro", "Turn"),
                               bg=C["card"], fg=C["text"], troughcolor=C["frame"], highlightthickness=0,
                               command=lambda _v: self._programar_render())
        self.sc_yaw.pack(fill="x", padx=10, pady=(0, 10))

        # (3) receta
        right = tk.Frame(body, bg=C["card"])
        right.grid(row=0, column=2, sticky="nsew")
        pad = dict(anchor="w", padx=12, pady=(10, 0))
        label(right, T("Fuente (animación que se pone)", "Source (animation to use)"), fg=C["dim"]).pack(fill="x", **pad)
        self.lbl_fuente = label(right, T("Ninguna", "None"), wrap=260)
        self.lbl_fuente.pack(fill="x", padx=12)
        label(right, T("Destino (acción que se reemplaza)", "Target (action to replace)"), fg=C["dim"]).pack(fill="x", **pad)
        self.lbl_destino = label(right, T("Ninguno: elige una fila y pulsa «Usar como destino»",
                                          "None: pick a row and press «Use as target»"), wrap=260)
        self.lbl_destino.pack(fill="x", padx=12)
        ttk.Button(right, text=T("Usar como destino", "Use as target"), style="Blue.TButton",
                   command=self._usar_destino).pack(fill="x", padx=12, pady=(12, 0))
        label(right, T("Nombre de la receta", "Recipe name"), fg=C["dim"]).pack(fill="x", **pad)
        self.var_nombre = tk.StringVar()
        tk.Entry(right, textvariable=self.var_nombre, bg=C["bg"], fg=C["text"], insertbackground=C["text"],
                 relief="flat", font=F["body"]).pack(fill="x", padx=12, ipady=4)
        ttk.Button(right, text=T("Guardar receta", "Save recipe"), style="Accent.TButton",
                   command=self._guardar).pack(fill="x", padx=12, pady=(14, 0))
        label(right, T("La receta solo se guarda en un fichero .toml. Aplicarla al mod todavía no está hecho.",
                       "The recipe is saved as a .toml file only. Applying it to a mod is not done yet."),
              fg=C["dim"], wrap=260).pack(fill="x", padx=12, pady=(10, 0))

    # ------------------------------------------------------------ datos
    def on_show(self):
        if not self._cargado and not getattr(self, "_hilo", None):
            self._hilo = threading.Thread(target=lambda: self._cola.put(("ok", bib.biblioteca())), daemon=True)
            self._hilo.start()
            self.after(100, self._sondear)

    def _sondear(self):
        if not self.alive:
            return
        try:
            kind, data = self._cola.get_nowait()
        except queue.Empty:
            self.after(100, self._sondear)
            return
        if kind == "ok":
            self.filas, self._cargado = data, True
            self._refrescar()
        else:
            self.status.configure(text=T("No se pudo leer el juego: %s", "Could not read the game: %s") % data,
                                  fg=C["err"])

    def cargar_sync(self):
        """Carga sincrona (selftest y uso sin hilos)."""
        self.filas, self._cargado = bib.biblioteca(), True
        self._refrescar()

    def _refrescar(self):
        if not self.filas:
            return
        i = self.cb_cat.current()
        cat = None if i <= 0 else self.cats[i]
        self.visibles = bib.buscar(self.filas, self.var_q.get(), cat)
        self.tree.delete(*self.tree.get_children())
        for k, f in enumerate(self.visibles[:3000]):
            self.tree.insert("", "end", iid=str(k), values=(
                "%d %s" % (f["personaje"], f["nombre"]),
                "%#05x" % f["codigo"] if f["codigo"] is not None else "-",
                f["anim"] if f["anim"] is not None else "-",
                f["frames"] if f["frames"] is not None else "-",
                f["categoria"]))
        extra = "" if len(self.visibles) <= 3000 else T(" (se muestran 3000: afina la búsqueda)",
                                                       " (only 3000 shown: refine the search)")
        self.status.configure(text=T("%d clips", "%d clips") % len(self.visibles) + extra, fg=C["dim"])

    # ------------------------------------------------------------ seleccion y preview
    def _al_seleccionar(self):
        s = self.tree.selection()
        if not s:
            return
        self.sel = self.visibles[int(s[0])]
        f = self.sel
        self.lbl_fuente.configure(text=self._texto(f))
        if f["anim"] is None:
            self.lbl_img.configure(image="", text=T("Este clip no está resuelto (banco %s).\nNo tiene vista previa.",
                                                    "This clip is not resolved (bank %s).\nNo preview.")
                                   % f["pool"])
            self._photo = None
            return
        n = f["frames"] or 1
        self.sc_frame.configure(to=max(0, n - 1))
        self.sc_frame.set(0)
        self._programar_render()

    def _programar_render(self):
        if self._pend:
            self.after_cancel(self._pend)
        self._pend = self.after(120, self._render)

    def _render(self):
        self._pend = None
        if not self.sel or self.sel["anim"] is None:
            return
        try:
            from PIL import Image, ImageTk  # noqa: PLC0415
            arr = bib.render_fotograma(self.sel["personaje"], self.sel["anim"], int(self.sc_frame.get()),
                                       size=(340, 340), yaw=float(self.sc_yaw.get()))
            self._photo = ImageTk.PhotoImage(Image.fromarray(arr))
            self.lbl_img.configure(image=self._photo, text="")
        except Exception as ex:  # noqa: BLE001
            self._photo = None
            self.lbl_img.configure(image="", text=T("No se pudo dibujar: %s", "Could not draw: %s") % ex)

    # ------------------------------------------------------------ receta
    def _texto(self, f):
        cod = "%#05x" % f["codigo"] if f["codigo"] is not None else "-"
        return "%d %s · %s · %s" % (f["personaje"], f["nombre"], cod, f["categoria"])

    def _usar_destino(self):
        if not self.sel or self.sel["codigo"] is None:
            messagebox.showwarning(T("Biblioteca", "Library"),
                                   T("Elige primero una fila con código BSK.", "Pick a row with a BSK code first."))
            return
        self.destino = dict(self.sel)
        self.lbl_destino.configure(text=self._texto(self.destino))

    def _guardar(self):
        if not self.sel or not self.destino:
            messagebox.showwarning(T("Biblioteca", "Library"),
                                   T("Falta la fuente (clip de la lista) o el destino («Usar como destino»).",
                                     "Missing the source (list clip) or the target («Use as target»)."))
            return
        nombre = self.var_nombre.get().strip() or "receta"
        ruta = filedialog.asksaveasfilename(defaultextension=".toml", initialfile=nombre + ".toml",
                                            filetypes=[("Receta", "*.toml")])
        if ruta:
            self.guardar_en(ruta, nombre)

    def guardar_en(self, ruta, nombre="receta"):
        """Escribe la receta en `ruta` (sin dialogo). -> ruta."""
        d, s = self.destino, self.sel
        bib.guardar_receta(ruta,
                           dict(personaje=d["personaje"], codigo=d["codigo"], anim=d["anim"]),
                           dict(personaje=s["personaje"], codigo=s["codigo"], anim=s["anim"]),
                           nombre=nombre)
        self.status.configure(text=T("Receta guardada en %s", "Recipe saved to %s") % ruta, fg=C["ok"])
        return ruta


# ================================================================ selftest headless
def _selftest():
    ok = []

    def chk(nombre, cond, extra=""):
        ok.append(bool(cond))
        print("[%s] %s %s" % ("OK" if cond else "FALLO", nombre, extra))

    import tempfile  # noqa: PLC0415
    root = tk.Tk()
    root.withdraw()
    try:
        init_fonts(root)
        setup_style(root)

        class _App:
            env = None
            alive = True

            def goto(self, _k):
                pass

        app = _App()
        app.root = root
        pg = BibliotecaPage(root, app, True)
        pg.pack()
        root.update_idletasks()
        chk("pagina construida", pg.winfo_exists())
        pg.cargar_sync()
        total = len(pg.filas)
        chk("biblioteca cargada", total > 500, "(%d filas)" % total)
        pg.var_q.set("gokou")
        chk("busqueda por nombre en la lista (Gokou y Gokou S)", len(pg.tree.get_children()) > 0 and all(
            f["personaje"] in (0, 1) for f in pg.visibles), "(%d)" % len(pg.visibles))
        pg.var_q.set("")
        pg.cb_cat.current(1 + bib.CATEGORIAS.index("ataque"))
        pg._refrescar()
        chk("filtro categoria ataque", pg.visibles and all(f["categoria"] == "ataque" for f in pg.visibles))
        pg.var_q.set("gokou")
        pg.cb_cat.current(0)
        pg._refrescar()
        fila = next(i for i, f in enumerate(pg.visibles) if f["anim"] is not None and f["frames"])
        pg.tree.selection_set(str(fila))
        pg._al_seleccionar()
        chk("seleccion fija la fuente", pg.sel is pg.visibles[fila])
        pg.sc_frame.set(min(3, pg.sel["frames"] - 1))
        pg._render()
        chk("preview renderiza un fotograma", pg._photo is not None and pg.lbl_img.cget("image") != "")
        pg._usar_destino()
        chk("destino fijado", pg.destino is not None and pg.destino["codigo"] == pg.sel["codigo"])
        with tempfile.TemporaryDirectory() as td:
            ruta = os.path.join(td, "r.toml")
            pg.guardar_en(ruta, "selftest")
            r = bib.leer_receta(ruta)
            chk("receta guardada y legible", r["fuente"]["personaje"] == pg.sel["personaje"])
            chk("receta valida contra la biblioteca", bib.validar_receta(r, pg.filas) == [])
        pg.destroy()
    finally:
        root.destroy()
    print("RESULTADO PAGINA: %d/%d OK" % (sum(ok), len(ok)))
    return all(ok)


if __name__ == "__main__":
    if "--selftest" in sys.argv:
        sys.exit(0 if _selftest() else 1)
    print(__doc__)
