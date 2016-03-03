import sys

f = open(sys.argv[1], 'rb')
res =  b'typedef struct {const char *vert, *geom, *frag;} shaderdata_t;\n'
res += b'const shaderdata_t shaderdata[] = {\n'
i = 0
a = False
for line in f:
    if not a:
        res += b'{\n'
        a = True
    if line == b'~\n':
        i += 1
        if i == 3:
            i = 0
            res += b'},\n'
            a = False
        else:
            res += b',\n'
    elif line == b'~~\n':
        res += b',\n0,\n'
        i += 2
    else:
        res += b'"' + line[:-1] + b'"\n'
res += b'};\n'
f.close()

f = open(sys.argv[2], 'wb')
f.write(res)
f.close()
