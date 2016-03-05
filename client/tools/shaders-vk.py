import sys
import subprocess
import glob
import os
import array

assert(len(sys.argv) == 5)

head = array.array('H')
out = bytearray()

header = 'enum {\n'

count = 0
i = 0
hasgeom = 0

for path in glob.glob(sys.argv[2] + '*.vert'):
    p = os.path.splitext(path)[0]
    name = os.path.basename(p)

    header += '    so_%s,\n' % name

    try:
        subprocess.check_call([sys.argv[1], '-s', '-V', '-o', 'tmp/' + name + '.vert', p + '.vert'])
    except subprocess.CalledProcessError:
        subprocess.check_call([sys.argv[1], '-V', '-o', 'tmp/' + name + '.vert', p + '.vert'])

    try:
        subprocess.check_call([sys.argv[1], '-s', '-V', '-o', 'tmp/' + name + '.frag', p + '.frag'])
    except subprocess.CalledProcessError:
        subprocess.check_call([sys.argv[1], '-V', '-o', 'tmp/' + name + '.frag', p + '.frag'])

    geom = 0
    try:
        f = open(p + '.geom', 'rb')
        f.close()
        subprocess.check_call([sys.argv[1], '-s', '-V', '-o', 'tmp/' + name + '.geom', p + '.geom'])
        geom = 1
    except IOError:
        pass

    f = open('tmp/' + name + '.vert', 'rb')
    data = f.read()
    f.close()

    out += data
    head.append(len(data))

    f = open('tmp/' + name + '.frag', 'rb')
    data = f.read()
    f.close()

    out += data
    head.append(len(data))

    if geom:
        f = open('tmp/' + name + '.geom', 'rb')
        data = f.read()
        f.close()

        out += data
        head.append(len(data))

    hasgeom |= (geom << i)

    count += 2 + geom
    i = i + 1

header += '    num_shader,\n'
header += '};\n'
header += '#define shader_hasgeom %d\n' % hasgeom
header += '#define shader_objcount %d\n' % count

f = open(sys.argv[3], 'wb')
f.write(head)
f.write(out)
f.close()

f = open(sys.argv[4], 'w')
f.write(header)
f.close()

print(header)
