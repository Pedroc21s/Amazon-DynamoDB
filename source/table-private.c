/*
* Grupo SD-41
* - Miguel Mendes fc59885
* - Pedro Correia fc59791
* - Sara Ribeiro fc59873
*/

#include "block.h"
#include "table.h"
#include "table-private.h"   
#include "list.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Este método decide o hashing
 */

int get_hashcode(char* key, int size) {
    int hash = 0;
    for (int i = 0; key[i] != '\0'; i++) {
        hash += (int) key[i];
    }
    return hash % size;
}
