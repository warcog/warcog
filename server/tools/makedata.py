# python script to create data files and header for map

import sys
import lzma
import math

assert(len(sys.argv) == 5)

f = open(sys.argv[1], 'rb')
tilemap = f.read()
f.close()

f = open(sys.argv[2], 'rb')
defs = f.read()
f.close()

svtile = bytearray()
cltile = bytearray()

tiles = [tilemap[i:i+8] for i in range(0, len(tilemap), 8)]
for t in tiles:
    svtile.append(t[2])
    svtile.append(t[3])
    svtile.append(t[4])
    svtile.append(t[5])

    cltile.append(t[0])
    cltile.append(t[1])

    cltile.append(t[2])
    cltile.append(t[3])
    cltile.append(t[4])
    cltile.append(t[5])

for t in tiles:
    svtile.append(t[6])


data = defs + cltile;

map_size = int(math.sqrt(len(tilemap) / 8))
map_shift = int(math.log(map_size, 2))

out = lzma.compress(data, check=lzma.CHECK_CRC32, preset=(9 or lzma.PRESET_EXTREME))

header = ''
header += '#define map_shift %u\n' % map_shift
header += '#define map_size (1 << map_shift)\n'
header += '#define compressed_size %u\n' % len(out)
header += '#define inflated_size %u\n' % len(data)

f = open(sys.argv[3] + 'datamap', 'wb')
f.write(svtile)
f.close()

f = open(sys.argv[3] + 'data', 'wb')
f.write(out)
f.close()

f = open(sys.argv[4], 'w')
f.write(header)
f.close()
