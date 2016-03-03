# python script to convert a map's string file to C code

import sys

assert(len(sys.argv) == 3)


text = 'const char text[] = '
head = 'enum {\n'
i = 0

f = open(sys.argv[1], 'r')
for line in f:
    if line[0] == '\n':
        continue;
    a, b = line.split(' ', 1)
    b = b[:-1]

    head += '    text_' + a + ' = ' + str(i) + ',\n'
    text += '"' + b + '\\0"\n';
    i += len(b.replace('\\', '')) + 1

f.close()

text += ';\n'
head += '    text_size = ' + str(i) + ',\n};\n'

f = open(sys.argv[2] + 'text.c', 'w')
f.write(text)
f.close()

f = open(sys.argv[2] + 'text.h', 'w')
f.write(head)
f.close()
