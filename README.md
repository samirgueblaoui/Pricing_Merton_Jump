# Merton Jump-Diffusion

Projet C++ de pricing d’options européennes et bermudiennes sous un modèle de Merton-Jump.

Le programme simule les trajectoires du sous-jacent, estime les prix des calls et des puts, puis affiche une trajectoire avec Matplotlib.

## Fonctionnalités

- Simulation du modèle de Merton avec compensation des sauts dans la dérive risque neutre.
- Valorisation des options européennes par Monte-Carlo.
- Valorisation des options bermudiennes par Longstaff–Schwartz, avec régression polynomiale et réduction du degré en cas de données insuffisantes ou dégénérées.
- Contrôle des paramètres.
- Tests de non-régression avec AddressSanitizer et UndefinedBehaviorSanitizer.

## Organisation

```text
Merton_Jump/
├── include/
│   ├── MertonModel.hpp        # Interfaces et modèle de Merton
│   ├── MonteCarloPricer.hpp   # Moteurs de valorisation
│   └── Option.hpp            # Options européennes et bermudiennes
├── src/
│   ├── main.cpp              # Exemple d’utilisation
│   ├── MertonModel.cpp
│   ├── MonteCarloPricer.cpp
│   └── Option.cpp
├── tests/
│   ├── regression.cpp
│   └── run_tests.sh
├── Makefile                  # Compilation, exécution et tests
└── README.md
```

## Prérequis

Le Makefile est configuré par défaut pour macOS avec Anaconda installé dans `/opt/anaconda3`. Toutes les commandes s’utilisent dans un terminal.

- Un compilateur compatible C++20.
- `make` et un shell compatible POSIX.
- Python 3.11, ses en-têtes de développement et la bibliothèque `libpython3.11`.
- NumPy et Matplotlib dans cet environnement Python.
- L’en-tête C++ `matplotlibcpp.h`.

## Compilation et exécution

Exécuter les commandes suivantes depuis la racine du projet.

### Version optimisée

```sh
make release
./output_release
```

### Version de débogage

```sh
make debug
./output_debug
```

Le programme affiche quatre prix : call européen, put européen, call bermudien et put bermudien. Il ouvre ensuite le graphique d’une trajectoire ; selon le backend Matplotlib, il faut fermer la fenêtre pour terminer l’exécution.

`make` compile par défaut la version optimisée. `make run` compile cette version si nécessaire puis lance le programme. `make clean` supprime les exécutables générés et leurs symboles de débogage.



## Paramètres de l’exemple

Les paramètres se modifient directement dans [src/main.cpp](src/main.cpp). Il n’y a pas d’arguments de ligne de commande.

```cpp
MertonModel model(90.0, 0.05, 0.2, 5.0, -0.1, 0.3, 40000);
```

| Paramètre | Signification | Valeur de l’exemple |
| --- | --- | --- |
| `S0` | Prix initial du sous-jacent | `90.0` |
| `r` | Taux sans risque annuel | `0.05` |
| `sigma` | Volatilité de la diffusion | `0.2` |
| `lambda` | Intensité annuelle des sauts | `5.0` |
| `muJ` | Moyenne du logarithme du multiplicateur de saut | `-0.1` |
| `sigmaJ` | Écart-type de ce logarithme | `0.3` |
| `nPath` | Nombre de pas de temps par trajectoire | `40000` |


## Reproductibilité

Par défaut, le générateur est initialisé aléatoirement. Pour reproduire les calculs du programme dans le même environnement, fixer la graine avant le clonage :

```cpp
MertonModel model(90.0, 0.05, 0.2, 5.0, -0.1, 0.3, 400);
model.setSeed(42);
std::shared_ptr<AbstractModel> model_ptr = model.clone();
```

## Tests

```sh
make test
```