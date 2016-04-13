<meta charset="utf-8">

# TER - Visualisation rapide de modèles 3D

Le dépôt est composé de plusieurs applications :

* capture : permet de capturer un ensemble de points de vue d'un objet
  * Rotation autour de l'objet : souris
  * Déplacement : ZQSD (horizontal), RF (vertical)
  * Zoom : +/-
  * Enregistrer un point de vue : A
  * Enregistrer l'ensemble de points de vue : E
  * Quitter : &lt;ESC&gt;
* vdtm : application de la technique de view-dependent texture mapping à partir
  de textures projectives de profondeur acquises par l'application `capture`

## Dépendances

* Dépendances du moteur
* Boost
  * serialize
  * program_options
