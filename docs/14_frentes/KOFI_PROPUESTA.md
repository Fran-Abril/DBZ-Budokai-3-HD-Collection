# Propuesta de Ko-fi para las herramientas (borrador, 2026-10-16)

No es asesoramiento legal. Antes de activarlo, conviene consultarlo con un abogado de propiedad intelectual.
Investigación del enjambre Haiku, con fuentes: ver el final.

## Conclusión corta
- **Se puede hacer.** GitHub permite el botón "Sponsor" con `.github/FUNDING.yml` y permite enlaces de donación relacionados con el proyecto.
- **Hay riesgo, aunque el texto ayuda.** El texto que pegaste dice que el blindaje es "óptimo"; es demasiado optimista.
  - En los precedentes, lo que más acerca un aviso de retirada es el dinero y la prensa: lo dijo un exabogado de Pokémon tras el caso Pokémon Uranium.
  - No hay ningún caso documentado de "donaciones solo para herramientas" que haya protegido a un proyecto de fans.
  - RPCS3 tiene Patreon desde 2017; Atlus solo le hizo quitar la mención a Persona 5.
  - Dolphin no acepta donaciones.
- **Bandai Namco** no tiene una guía pública de obras de fans; su postura sobre monetización no está confirmada.
- **Lo que más reduce el riesgo:**
  - poner las herramientas (launcher, kit de mods, conversores, el framework) en un repositorio propio, sin Dragon Ball en el nombre;
  - que el Ko-fi apunte solo a ese repositorio.

## Estado del repo
- Licencia MIT.
- El README (líneas ~146-148) ya tiene el aviso de no afiliación.
- No hay `FUNDING.yml`.
- El arte oficial de `docs/13_assets_externos/` nunca debe salir en el Ko-fi.

## Borradores (para cuando tengas la cuenta)

`.github/FUNDING.yml` (solo el nombre de usuario):
```yaml
ko_fi: TU_USUARIO_KOFI
```

Sección del README:
> ## Apoyo al desarrollo de herramientas
> Las herramientas de este proyecto (launcher, kit de mods, conversores de formatos y el framework de recompilación) son software libre bajo licencia MIT. Si te resultan útiles para la preservación y el modding de juegos clásicos, puedes apoyar su desarrollo en [Ko-fi](https://ko-fi.com/TU_USUARIO_KOFI). Las aportaciones financian solo esas herramientas de código abierto, nunca el juego ni material de terceros.

Aviso legal (README y Ko-fi):
> Proyecto no oficial y sin ánimo de lucro, de investigación y preservación.
> No está afiliado, respaldado ni patrocinado por Bandai Namco, Shueisha, Toei Animation ni los titulares de Dragon Ball. Las marcas y los personajes pertenecen a sus titulares.
> El repositorio no contiene datos del juego: cada usuario aporta su propia copia obtenida legalmente.

Descripción de la página de Ko-fi:
> Herramientas de código abierto para modding y recompilación de juegos clásicos de consola: launcher, kit de mods, conversores de formatos y framework de recompilación. Esta página financia solo el desarrollo de esas herramientas. No vende ni distribuye juegos ni material protegido, y no es una página oficial de ningún titular de derechos.

## Qué sí y qué no
**Sí:**
- título, avatar y portada genéricos (tu nombre o el del framework);
- licencia visible;
- aviso legal;
- recompensas genéricas (agradecimiento, nombre en los créditos de las herramientas).

**No:**
- capturas, personajes, logos, tipografías o música de Dragon Ball en el Ko-fi;
- "Dragon Ball" en el título, la URL o las etiquetas;
- ventajas ligadas al port: acceso anticipado a personajes, builds o packs;
- mods de pago;
- llamarlo "oficial".

## Fuentes
- GitHub, botón de patrocinio: https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/displaying-a-sponsor-button-in-your-repository
- GitHub, uso aceptable (§10 publicidad): https://docs.github.com/en/site-policy/acceptable-use-policies/github-acceptable-use-policies
- Ko-fi, copyright: https://help.ko-fi.com/hc/en-us/articles/10598202486429-Copyright-Guidelines (los términos completos dieron 403: sin confirmar).
- RPCS3 y Atlus: https://torrentfreak.com/of-course-atlus-hit-rpcs3s-patreon-page-over-persona-5-170927/
- Dolphin, sin donaciones: https://forums.dolphin-emu.org/Thread-patreonized
- Riesgo del dinero y la prensa: https://videogameschronicle.com/news/former-pokemon-lawyer-says-fundraising-and-press-coverage-lead-to-fan-project-takedowns
- AM2R: https://www.nintendolife.com/news/2016/09/nintendo_of_america_issues_takedown_request_on_am2r_ending_the_project
