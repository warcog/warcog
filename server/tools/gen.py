
# python script to preprocess map code
# many fixes/improvements to be made

import sys
import re

def readblock(x):
    k = 1
    for i, c in enumerate(x):
        if c == '}':
            k = k - 1
            if not k:
                return x[:i], x[i+1:]
        elif c == '{':
            k = k + 1
    raise ValueError('no closing } to block')

assert(len(sys.argv) >= 4)

head = ''
code = ''

typen = ['entity', 'ability', 'state', 'effect']
typename = {key : i for i, key in enumerate(typen)}
effn = ['particle', 'strip', 'sound', 'groundsprite', 'mdlmod', 'attachment']
effname = {key : i for i, key in enumerate(effn)}
function = [
[
    ["void", "onframe", "entity_t *this", "this"],
    ["void", "onspawn", "entity_t *this", "this"],
    ["void", "ondeath", "entity_t *this, entity_t *source", "this, source"],
    ["bool", "canmove", "entity_t *this", "this"],
    ["void", "initmod", "entity_t *this", "this"],
    ["void", "finishmod", "entity_t *this", "this"],
    ["bool", "visible", "entity_t *this, entity_t *ent", "this, ent"],
    ["uint32_t", "ondamaged",  "entity_t *this, entity_t *source, uint32_t amount, uint8_t type",
     "this, source, amount, type"],
],
[
    ["void", "onframe", "entity_t *self, ability_t *this", "self, this"],

    ["bool", "allowcast", "entity_t *self, ability_t *this, target_t target",
    "self, this, target"],
    ["cast_t", "startcast", "entity_t *self, ability_t *this, target_t *target",
    "self, this, target"],
    ["bool", "continuecast", "entity_t *self, ability_t *this, target_t *target",
    "self, this, target"],

    ["void", "onpreimpact", "entity_t *self, ability_t *this, target_t target",
    "self, this, target"],
    ["bool", "onimpact", "entity_t *self, ability_t *this, target_t target",
    "self, this, target"],

    ["bool", "ondeath", "entity_t *self, ability_t *this, entity_t *source", "self, this, source"],
    ["void", "onstateexpire", "entity_t *self, ability_t *this, state_t *s", "self, this, s"],

    ["void", "onabilitystart", "entity_t *self, ability_t *this, ability_t *a, target_t target",
    "self, this, a, target"],
    ["void", "onabilitypreimpact", "entity_t *self, ability_t *this, ability_t *a, target_t target",
    "self, this, a, target"],
    ["void", "finishmod", "entity_t *self, ability_t *this", "self, this"],
],
[
    ["void", "onframe", "entity_t *self, state_t *this", "self, this"],
    ["void", "onexpire", "entity_t *self, state_t *this", "self, this"],
    ["void", "onabilityimpact", "entity_t *self, state_t *this, ability_t *a", "self, this, a"],

    ["void", "onanimfinish", "entity_t *self, state_t *this", "self, this"],
    ["bool", "ondeath", "entity_t *self, state_t *this, entity_t *source", "self, this, source"],

    ["void", "ondamage",
    "entity_t *self, state_t *this, entity_t *target, uint32_t amount, uint8_t type",
    "self, this, target, amount, type"],
    ["uint32_t", "ondamaged",
    "entity_t *self, state_t *this, entity_t *source, uint32_t amount, uint8_t type",
    "self, this, source, amount, type"],

    ["void", "applymod", "entity_t *self, state_t *this", "self, this"],
]
]

data = [{}, {}, {}, {}]
effcount = [0, 0, 0, 0, 0, 0]

for path in sys.argv[3:]:
    f = open(path, 'r')
    a = f.read().strip()
    f.close()

    while len(a):
        t, a = a.split(':', 1)
        name, b = re.split('[:{]', a, 1)
        parent = None
        if a[len(name)] == ':':
            parent, b = b.split('{', 1)
            parent = parent.strip()

        t = t.strip()
        name = name.strip()
        b, a = readblock(b)

        tid = typename[t]
        fn = {}
        struct = []
        private = name.endswith('*')
        if private:
            name = name[:-1]

        b = b.strip()
        while len(b):
            c, b = b.split('{', 1)
            c = c.strip()
            if not len(c):
                info, b = readblock(b)
            else:
                if tid == 3:
                    content, b = readblock(b)
                    i = effname[c]
                    j = effcount[i]
                    effcount[i] += 1
                    struct += [(i, j, content.replace('\n', '\\\n'))]
                else:
                    x, y = c.split('(',1)
                    fname = x.split(' ',1)[1]
                    code, b = readblock(b)
                    fn[fname] = (y.split(')',1)[0], code)
        d = [name, private, parent, info.replace('\n', '\\\n'), fn, struct]

        data[tid][name] = d

code =  '#include "../warlock/main.h"\n'
code += '#include <assert.h>\n'

head = ''
func = ''

code += '#ifndef BUILD_DEFS\n'

for i, d in enumerate(data):
    head += 'enum {\n'
    for key, g in d.items():
        if not g[1]:
            if i == 0:
                head += '    %s,\n' % g[0]
            else:
                head += '    %s_%s,\n' % (typen[i], g[0])
    head += '};\n'

for i, d in enumerate(data[:3]):
    for key, g in d.items():
        for f in function[i]:
            code += '%s %s_%s_%s(game_t *g, %s);\n' % (f[0], typen[i], g[0], f[1], f[2])

for i, d in enumerate(data[:3]):
    for key, g in d.items():
        if g[2]:
            code += '#define _parent %s_%s\n' % (typen[i], g[2])

        for f in function[i]:
            try:
                a, c = g[4][f[1]]
                code += '%s %s_%s_%s(%s)\n' % (f[0], typen[i], g[0], f[1], a)
                code += '#define _function %s\n{%s}\n#undef _function\n' % (f[1], c)
            except KeyError:
                code += '%s %s_%s_%s(game_t *g, %s)\n{\n' % (f[0], typen[i], g[0], f[1], f[2])
                code += '    return %s_%s_%s(g, %s);\n}\n\n' % (typen[i], g[2], f[1], f[3])

        if g[2]:
            code += '#undef _parent\n'

for i, d in enumerate(function):
    for f in d:
        func += '%s %s_%s(game_t *g, %s);\n' % (f[0], typen[i], f[1], f[2])

        code += '%s %s_%s(game_t *g, %s)\n{\n' % (f[0], typen[i], f[1], f[2])
        code += '    if (this->def < 0) return %s_base_%s(g, %s);\n' % (typen[i], f[1], f[3])
        code += '    switch (this->def) {\n'

        for key, g in data[i].items():
            if not g[1]:
                code += '    case %s%s:\n' % ([typen[i] + '_', ''][not i], g[0])
                code += '        return %s_%s_%s(g, %s);\n' % (typen[i], g[0], f[1], f[3])

        code += '    default: assert(0); __builtin_unreachable();\n    }\n}\n\n'

code += '#endif\n'

hashes = []

def hash_name(s):
    res = [0, 0]
    for i, c in enumerate(s):
        res[i & 1] += ord(c)
    return (res[0] & 0xFF) | ((res[1] & 0xFF) << 8)

for i, d in enumerate(data[:3]):
    for key, g in d.items():
        if 'server:' in g[3]:
            a, b = g[3].split('server:')
        else:
            a = g[3]
            b = ''

        if i == 1:
            h = hash_name(g[0])
            if h in hashes:
                raise ValueError('duplicate hash')
            hashes += [h]
            a = ('.hash = %u, ' % h) + a

        p = ('%s_%s_info' % (typen[i], g[2])) if g[2] else ''
        code += '#define %s_%s_info %s %s\n' % (typen[i], g[0], p, a)
        p = ('%s_%s_sinfo' % (typen[i], g[2])) if g[2] else ''
        code += '#define %s_%s_sinfo %s %s\n' % (typen[i], g[0], p, b)

for key, g in data[3].items():
    for (j, k, info) in g[5]:
        #p = ('%s_%u_info' % (effn[j], k)) if g[2] else ''
        code += '#define %s_%u_info %s\n' % (effn[j], k, info)

for i, d in enumerate(function):
    code += 'const %ssdef_t %ssdef[] = {\n' % (typen[i], typen[i])
    for key, g in data[i].items():
        if not g[1]:
            code += '    {%s_%s_info %s_%s_sinfo},\n' % (typen[i], g[0], typen[i], g[0])

    code += '};\n'

code += '#ifdef BUILD_DEFS\n'

for i, d in enumerate(function):
    code += 'const %sdef_t %sdef[] = {\n' % (typen[i], typen[i])
    for key, g in data[i].items():
        if not g[1]:
            code += '    {%s_%s_info},\n' % (typen[i], g[0])

    code += '};\n'

for (name, k) in zip(effn, effcount):
    code += 'const %sdef_t %sdef[] = {\n' % (name, name)
    for i in range(k):
        code += '    {%s_%u_info},\n' % (name, i)
    code += '};\n'

i = 3
code += 'const %sdef_t %sdef[] = {\n' % (typen[i], typen[i])
for key, g in data[i].items():
    code += '{'
    for (j, k, info) in g[5]:
        code += '{{%u, %u}},' % (j + 1, k)
    code += '},\n'
code += '};\n'


code += 'const mapdef_t mapdef = {.size = map_shift,.map_id=%u,.ndef = {\n' % hash_name(sys.argv[2])
for s in (typen + effn):
    code += ('countof(%sdef),' % s)
code += '},\n'
code += '    .textlen = text_size,\n'
code += '    .name = map_name,\n'
code += '    .desc = map_desc,\n'
code += '    .tileset = {map_tileset, -1},\n'
code += '    .iconset = {map_iconset, -1},\n'
code += '};\n'

code += '#endif\n'

f = open(sys.argv[1] + 'gen.c', 'w')
f.write(code)
f.close()

f = open(sys.argv[1] + 'gen.h', 'w')
f.write(head)
f.close()

f = open(sys.argv[1] + 'functions.h', 'w')
f.write(func)
f.close()
