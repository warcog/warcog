# script to convert custom markdown to xhtml

import sys
import os

def replace(s, ch, f):
    for i, a in enumerate(s.split(ch)):
        if i == 0:
            res = a
        else:
            res += f(i - 1) + a
    return res

head = """
<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
 "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<title>
""".replace('\n', '')

head2 = """
</title>
<style>
html{
margin: 0;
padding: 0;
}
body{
margin: 1em 5% 1em 5%;
font-family: tahoma, veranda, sans-serif;
font-size: 1.0em;
}
div{
margin: 0;
}
h1,h2{
color: #527bbd;
border-bottom: 2px solid silver;
line-height: 1.3;
}
code,pre{
font-family: "courier new", monospace;
}
code{
color: navy;
}
pre{
border: 1px solid silver;
background: #f4f4f4;
margin: 0.5em 10% 0.5em 0;
padding: 0.5em 1em;
font-family: "courier new", monospace;
line-height: 1.0;
color: #333333;
}
</style>
</head>

<body>
""".replace('\n', '')

foot = """
</body>
</html>
""".replace('\n', '')

assert(len(sys.argv) >= 3)

for path in sys.argv[2:]:
    name = os.path.basename(os.path.splitext(path)[0])

    f = open(path, "r")
    s = f.read()
    f.close()

    out = '%swarcog - %s%s' % (head, name, head2)
    need_newline = 0

    while len(s):
        if s.startswith('```\n'):
            a, s = s[4:].split('\n```', 1)
            out += '<pre>%s</pre>' % a
            need_newline = 0

        a, s = s.split('\n', 1)

        b = a.lstrip('#')
        h = len(a) - len(b)

        a = b.lstrip(' ')
        t = len(b) - len(a)

        a = replace(a, '`', lambda x: ['<code>', '</code>'][x % 2])

        if len(a):
            out += '<h%u>%s</h%u>' % (h, a, h) if h else ['', '<br />'][need_newline] + \
                   ('<span style="margin-left:%uem">%s</span>' % (t, a) if t else a)
            need_newline = not h

    out += foot

    f = open(sys.argv[1] + name + '.html', "wb")
    f.write(out.encode('utf-8'))
    f.close()
