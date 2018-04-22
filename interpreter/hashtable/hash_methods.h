#pragma once

#include <stdio.h>
#include "../VM.h"
#include "../opcode.h"
#include "../constant/constant.h"
#include "hashtable.h"
#include "../list/list.h"

int map_keys(VM *vm);

int map_values(VM *vm);