# Plan d'amélioration du convertisseur PMD vers glTF

## Améliorations identifiées depuis l'analyse de pmd2collada

### 1. Transformation des coordonnées
**Priorité : HAUTE**
- Implémenter la conversion des coordonnées de jeu vers un système right-handed Z_UP
- Transformation requise : `[x, y, z] → [x, z, y]` pour positions et normales
- Cette transformation annule ce qui est fait dans PMDConverter.cpp

### 2. Gestion des versions PMD
**Priorité : HAUTE**
- Version 1 : Pas de prop points
- Version 2+ : Support des prop points
- Versions ≤2 : Les vertices sont stockés relatifs au bind-pose et nécessitent une transformation spéciale

### 3. Structures de données complètes
**Priorité : MOYENNE**
- **Vertex** : Vérifier que nous avons position, normal, UV, blends (4 bones + 4 weights)
- **Face** : 3 indices de vertices
- **Bone** : translation (Vector3D) + rotation (Quaternion)
- **PropPoint** : nom, translation, rotation, bone parent

### 4. Fonctions mathématiques avancées
**Priorité : HAUTE**
- Implémenter conversion Quaternion → Matrix44
- Ajouter calcul d'inverse de matrice
- Implémenter multiplication de matrices
- Ajouter transformation de vecteurs

### 5. Gestion des prop points
**Priorité : MOYENNE**
- Ajouter préfixe `prop_` aux prop points internes
- Ignorer le prop point par défaut "root" et ceux avec bone == 255
- Placer les prop points comme enfants du bone correct

### 6. Calculs de bones avancés
**Priorité : HAUTE**
- Implémenter la hiérarchie des bones (parents, -1 pour root)
- Calculer les "inverse bind poses"
- Pour version ≤2 : appliquer le bind pose aux vertices

### 7. Validation et robustesse
**Priorité : MOYENNE**
- Ajouter des checks de version PMD
- Valider les données lors du parsing
- Améliorer la gestion d'erreurs

### 8. Debug et information
**Priorité : BASSE**
- Ajouter des messages informatifs comme :
  `Valid PMDv{version}: Verts={n}, Faces={n}, Bones={n}, Props={n}`

## Actions à implémenter

1. **Phase 1 - Corrections critiques**
   - [ ] Ajouter transformation des coordonnées dans pmd_parser.c
   - [ ] Implémenter les fonctions mathématiques de base (Matrix44, Quaternion)
   - [ ] Corriger la gestion des versions PMD

2. **Phase 2 - Améliorations structurelles**
   - [ ] Améliorer la gestion des prop points
   - [ ] Implémenter les calculs de bind poses
   - [ ] Ajouter validation des données

3. **Phase 3 - Finalisation**
   - [ ] Optimiser les performances
   - [ ] Améliorer les messages de debug
   - [ ] Tests complets avec différentes versions PMD

## Nouvelles découvertes - Implémentation C++ officielle

### Format PMD découvert dans le code C++
**Priorité : CRITIQUE**
Le code C++ révèle le vrai format PMD v4 :
```cpp
output("PSMD", 4);  // magic number "PSMD"
write(output, (uint32)4); // version number = 4
```

### Structure des vertices avancée
- Support de **plusieurs sets UV** par vertex
- Format : `position[3] + normal[3] + UV_sets + bone_weights`
- Le nombre d'UV sets est écrit avant les données vertex

### Transformations de coordonnées précises
**Deux cas selon up_axis :**
- **Y_UP** : `pos.z = -pos.z; norm.z = -norm.z`
- **Z_UP** : `swap(pos.y, pos.z); swap(norm.y, norm.z)`

### Prop points - conversion quaternions Z_UP
```cpp
// Pour Z_UP, conversion spéciale des quaternions :
swap(orientation[1], orientation[2]);
orientation[3] = -orientation[3];
```

### Gestion des bones avancée
- Support du **bind-shape matrix**
- Cas spécial : `bone ID = jointCount` pour vertices sans influence
- Limite : 254 bones maximum (0xFF réservé)

### Version du format PMD réelle
- **Version 4** avec magic "PSMD"
- Support multi-UV natif
- Structure : header + vertices + faces + bones + proppoints

## Corrections critiques identifiées

1. **Magic number incorrect** - doit être "PSMD" pas "PMD!" ✅ **CORRIGÉ**
2. **Version manquée** - nous implémentons v2/3 mais le vrai format est v4 ✅ **DÉJÀ SUPPORTÉ**
3. **Multi-UV manquant** - le vrai format supporte plusieurs sets UV ✅ **DÉJÀ SUPPORTÉ**
4. **~~Transformations de coordonnées~~** - ❌ **ERREUR** : PMD stocke déjà en world space
5. **Validation et robustesse** - ✅ **AJOUTÉE**

## 🚨 **Découverte importante**
Le format PMD stocke **déjà les coordonnées en world space** selon la documentation officielle. Les transformations du code C++ PMDConvert.cpp s'appliquent lors de la **création** du PMD depuis COLLADA, pas lors de la **lecture** du PMD !

## Références
- **Code source Python** : `docs/pmd2collada/pmd2collada.py`
- **Définitions de formats** : `docs/pmd2collada/pmd2collada_defs.py`
- **Fonctions mathématiques** : `docs/pmd2collada/vector_defs.py`
- **⭐ Implémentation C++ officielle** : `docs/collada/PMDConvert.cpp` (RÉFÉRENCE PRINCIPALE)
- **Structures communes** : `docs/collada/CommonConvert.h`
- **Fonctions mathématiques C++** : `docs/collada/Maths.cpp`