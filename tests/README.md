# Tests

Ce répertoire contient la suite de tests unitaires et d'intégration pour le convertisseur PMD vers glTF.

## Structure des tests

- `test_framework.h` - Framework de test personnalisé avec macros d'assertion
- `test_filesystem.c` - Tests pour les operations de système de fichiers
- `test_animation.c` - Tests pour l'extraction des noms d'animation
- `test_types.c` - Tests pour les structures de données (Vector3D, Quaternion, etc.)
- `run_tests.sh` - Script pour exécuter tous les tests manuellement

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

### Tests d'intégration  
- **integration_converter_test** : Test complet de conversion PMD vers glTF

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