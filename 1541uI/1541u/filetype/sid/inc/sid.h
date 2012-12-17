#ifndef SID_H
#define SID_H

#include "dir.h"
#include "ff.h"

FRESULT sid_dir(FIL *sid_file, DIRECTORY *directory);
void sid_load(FIL *f, BYTE song);

#endif