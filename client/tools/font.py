import sys
from struct import *

res = '#include "text.h"\nconst fontdata_t fontdata[] = {\n'

contours = ''
points = ''

f = open(sys.argv[1], 'rb')

d = f.read(8)
h, sw, y, unused = unpack('HHHH', d)

#res += '{ ' + str(h) + ', ' + sw + ', ' + y + ', 0,\n{\n'
res += '{ %u, %u, %u, 0,\n{\n' % (h, sw, y)

for i in range(94):
    d = f.read(4)
    ad, np, nc = unpack('HBB', d)
    res += '{%u, %u, %u},\n' % (ad, np, nc)

    for j in range(nc):
        d = f.read(1)
        c = unpack('B', d)
        contours += '%u,\n' % (c)

    for j in range(np):
        d = f.read(4)
        x, y = unpack('hh', d)
        points += '{%u, %u},\n' % (x, y)

res += '}\n}\n'

res += '};\n'

res += 'const uint8_t fontdata_contours[] = {' + contours + '};\n'
res += 'const point_t fontdata_points[] = {' + points + '};\n'

#print f.tell()
f.close()

f = open(sys.argv[2], 'wb')
f.write(res.encode('utf-8'))
f.close()
