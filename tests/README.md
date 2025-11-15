# Tests

Ce répertoire contient la suite de tests unitaires et d'intégration pour le convertisseur PMD vers glTF.

## Documentation

Pour une documentation complète sur la suite de tests, voir [docs/test-suite.md](../docs/test-suite.md) qui inclut:
- Spécifications techniques des fichiers de test
- Spécifications fonctionnelles de validation
- Description détaillée des cas de test
- Critères de validation

## Structure des tests

- `test_framework.h` - Framework de test personnalisé avec macros d'assertion
- `test_filesystem.c` - Tests pour les operations de système de fichiers
- `test_animation.c` - Tests pour l'extraction des noms d'animation
- `test_types.c` - Tests pour les structures de données (Vector3D, Quaternion, etc.)
- `test_pmd_cubes.c` - Tests d'intégration pour les cubes de test PMD
- `test_horse_model.c` - Tests d'intégration pour le modèle du cheval
- `test_gltf_output.c` - Tests de validation de la sortie glTF
- `generate_test_data.c` - Générateur de fichiers PMD/PSA de test
- `run_tests.sh` - Script pour exécuter tous les tests manuellement

### Fichiers de test générés

Les fichiers suivants sont générés automatiquement par `generate_test_data`:
- `data/cube_nobones.pmd` - Cube 2m×2m×2m sans squelette
- `data/cube_4bones.pmd` - Cube avec 4 os aux coins
- `data/cube_4bones_anim.psa` - Animation pour cube_4bones
- `data/cube_5bones.pmd` - Cube avec 5 os (4 coins + 1 centre)
- `data/cube_5bones_anim.psa` - Animation pour cube_5bones
- `data/cube_5bones.xml` - Squelette hiérarchique pour cube_5bones

## Exécution des tests

### Via CMake et CTest (recommandé)
```bash
# Configuration et compilation
cmake -B build
cmake --build build

# Exécuter tous les tests
cd build
ctest --output-on-failure

# Exécuter un test spécifique
ctest -R unit_filesystem --output-on-failure
```

### Exécution manuelle
```bash
# Compiler et exécuter tous les tests
./tests/run_tests.sh

# Ou exécuter individuellement
./build/test_filesystem
./build/test_animation  
./build/test_types
```

## Types de tests

### Tests unitaires
- **unit_filesystem** : Test des fonctions de recherche et énumération de fichiers
- **unit_animation** : Test de l'extraction des noms d'animation à partir des chemins
- **unit_types** : Test des structures de données et opérations de base
- **integration_pmd_cubes** : Tests de chargement et validation des cubes PMD de test
- **integration_horse_model** : Tests de validation du modèle du cheval (206 vertices, 33 bones, 8 prop points)

### Tests d'intégration  
- **integration_converter_test** : Test complet de conversion PMD vers glTF (horse)
- **integration_cube_nobones** : Conversion cube sans os vers glTF
- **integration_cube_4bones** : Conversion cube 4 os + animation vers glTF
- **integration_cube_5bones** : Conversion cube 5 os hiérarchique + animation vers glTF
- **validation_gltf_output** : Validation de la structure et du contenu des fichiers glTF générés

## Framework de test

Le framework de test personnalisé fournit les macros suivantes :

- `TEST_ASSERT(condition, message)` - Vérifie qu'une condition est vraie
- `TEST_ASSERT_EQ(expected, actual, message)` - Vérifie l'égalité de deux valeurs
- `RUN_TEST(test_func)` - Execute une fonction de test
- `run_tests(test_array, count)` - Execute un tableau de tests

## Intégration CI/CD

Tous les tests sont automatiquement exécutés dans le pipeline GitHub Actions sur :
- Linux (gcc et clang, Debug et Release)
- macOS (Debug et Release) 
- Windows (Debug et Release)

Les tests doivent passer sur toutes les plateformes pour que le build soit considéré comme réussi.