// write structs from map code into a file

#include <stdio.h>

#include "../defs.h"

#include <assert.h>

#define write(name, id) fwrite(name##def, sizeof(name##def_t), mapdef.ndef[id], file);

int main(int argc, char *argv[])
{
    FILE *file;

    assert(argc == 2);

    file = fopen(argv[1], "wb");
    if (!file)
        return -1;

    fwrite(&mapdef, sizeof(mapdef), 1, file);
    fwrite(entitydef, sizeof(entitydef_t), mapdef.ndef[0], file);
    fwrite(abilitydef, sizeof(abilitydef_t), mapdef.ndef[1], file);
    fwrite(statedef, sizeof(statedef_t), mapdef.ndef[2], file);

    write(effect, 3);
    write(particle, 4);
    write(strip, 5);
    write(sound, 6);
    write(groundsprite, 7);
    write(mdlmod, 8);
    write(attachment, 9);

    fwrite(text, 1, mapdef.textlen, file);

    return 0;
}
