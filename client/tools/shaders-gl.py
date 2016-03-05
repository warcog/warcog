import sys
import subprocess
import glob
import os
import array

assert(len(sys.argv) == 4)

head = array.array('H')
out = bytearray()

header = 'enum {\n'

count = 0
i = 0
hasgeom = 0

for path in glob.glob(sys.argv[1] + '*.vert'):
    p = os.path.splitext(path)[0]
    name = os.path.basename(p)

    header += '    so_%s,\n' % name

    f = open(p + '.vert', 'rb')
    data = f.read()
    f.close()

    data += b'\0'
    out += data
    head.append(len(data))

    f = open(p + '.frag', 'rb')
    data = f.read()
    f.close()

    data += b'\0'
    out += data
    head.append(len(data))

    geom = 0
    try:
        f = open(p + '.geom', 'rb')
        geom = 1
    except IOError:
        pass

    if geom:
        data = f.read()
        f.close()

        data += b'\0'
        out += data
        head.append(len(data))

    hasgeom |= (geom << i)
    count += 2 + geom
    i = i + 1

header += '    num_shader,\n'
header += '};\n'
header += '#define shader_hasgeom %d\n' % hasgeom
header += '#define shader_objcount %d\n' % count

f = open(sys.argv[2], 'wb')
f.write(head)
f.write(out)
f.close()

f = open(sys.argv[3], 'w')
f.write(header)
f.close()

print(header)
