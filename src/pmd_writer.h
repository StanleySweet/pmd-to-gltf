#ifndef PMD_WRITER_H
#define PMD_WRITER_H

#include "pmd_psa_types.h"
#include <stdio.h>

// Write PMD file to disk
int write_pmd(const char *filename, const PMDModel *model);

// Write PSA file to disk
int write_psa(const char *filename, const PSAAnimation *anim);

#endif
