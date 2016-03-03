# python script to create header file with map size

import sys
import math

assert(len(sys.argv) == 3)

f = open(sys.argv[1], 'rb')
tilemap = f.read()
f.close()

map_size = int(math.sqrt(len(tilemap) / 8))
map_shift = int(math.log(map_size, 2))

header = ''
header += '#define map_shift %u\n' % map_shift
header += '#define map_size (1 << map_shift)\n'
header += '#define compressed_size %u\n' % 0
header += '#define inflated_size %u\n' % 0

f = open(sys.argv[2], 'wb')
f.write(header.encode('utf-8'))
f.close()
