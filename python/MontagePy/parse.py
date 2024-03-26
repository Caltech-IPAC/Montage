import os
import glob
import json
from jinja2 import Template

MONTAGELIB = os.path.join('../..', 'MontageLib')

CTYPE = {}
CTYPE['int'] = 'int '
CTYPE['int*'] = 'int *'
CTYPE['integer'] = 'int '
CTYPE['char'] = 'char '
CTYPE['string'] = 'char *'
CTYPE['string*'] = 'char *'
CTYPE['boolean'] = 'int '
CTYPE['boolean*'] = 'int *'
CTYPE['double'] = 'double '
CTYPE['double*'] = 'double *'

PTYPE = {}
PTYPE['int'] = 'int'
PTYPE['int*'] = 'np.ndarray'
PTYPE['integer'] = 'int'
PTYPE['char'] = 'str'
PTYPE['string'] = 'str'
PTYPE['string*'] = 'str'
PTYPE['boolean'] = 'bool'
PTYPE['boolean*'] = 'np.ndarray'
PTYPE['double'] = 'float'
PTYPE['double*'] = 'np.ndarray'

with open('templates/template.pxd', 'r') as f:
    template_pxd = Template(f.read())

with open('templates/template.pyx', 'r') as f:
    template_pyx = Template(f.read())

with open('templates/template_main.pyx', 'r') as f:
    template_main_pyx = Template(f.read())



functions = []

for json_file in glob.glob(os.path.join(MONTAGELIB, '*', '*.json')):

    print("Parsing {0}...".format(json_file))

    with open(json_file, 'r') as fjson:
        data = json.load(fjson)

    if data['return'] is None:
        continue

    # We set up our own dictionary that we will then pass on to the jinja2
    # templates.
    function = {}

    # The name and description of the function
    function['name'] = data['function']
    function['summary'] = data['desc']

    # We now compile a list of arguments with defaults and arguments without
    # defaults - this is used in several places below. The arguments without
    # defaults come first.
    sorted_args = [arg for arg in data['arguments'] if 'default' not in arg]
    sorted_args += [arg for arg in data['arguments'] if 'default' in arg]

    # We now set up the lists of arguments.

    # Normal list of arguments
    function['arguments'] = [arg['name'] for arg in data['arguments']]

    # List of arguments to use in the C function declaration (functions
    # defined using cdef).
    function['arguments_cdef'] = []

    # List of arguments to use when calling the cdef wrapper from the normal
    # Cython function - this has to include converting string arguments to
    # bytes strings with .encode('ascii') and converting arrays with e.g.
    # .data.as_doubles.
    function['arguments_py_to_cdef'] = []

    for arg in data['arguments']:
        argument = "{0}{1}".format(CTYPE[arg['type']], arg['name'])
        function['arguments_cdef'].append(argument)

    function['arguments_with_defaults'] = []

    for arg in sorted_args:
        if 'default' in arg:
            function['arguments_with_defaults'].append(arg['name'] + '=' + repr(arg['default']))
        else:
            function['arguments_with_defaults'].append(arg['name'])

    function['array'] = []
    function['arguments_py_to_cdef'] = []
    for arg in data['arguments']:
        if arg['type'] == 'double*':
            function['array'].append("cdef _array.array {0}_arr = _array.array('d', {0})".format(arg['name']))
            function['arguments_py_to_cdef'].append('{0}_arr.data.as_doubles'.format(arg['name']))
        elif arg['type'] == 'int*':
            function['array'].append("cdef _array.array {0}_arr = _array.array('i', {0})".format(arg['name']))
            function['arguments_py_to_cdef'].append('{0}_arr.data.as_ints'.format(arg['name']))
        elif arg['type'] == 'boolean*':
            function['array'].append("cdef _array.array {0}_arr = _array.array('i', {0})".format(arg['name']))
            function['arguments_py_to_cdef'].append('{0}_arr.data.as_ints'.format(arg['name']))
        else:
            if 'string' in arg['type']:
                function['arguments_py_to_cdef'].append(arg['name'] + ".encode('ascii')")
            else:
                function['arguments_py_to_cdef'].append(arg['name'])

    function['struct_vars'] = []
    function['struct_vars_decl'] = []
    for ret in data['return']:
        struct_var = "{0}{1}".format(CTYPE[ret['type']], ret['name'])
        function['struct_vars'].append(ret['name'])
        function['struct_vars_decl'].append(struct_var)

    function['docstring_arguments'] = []
    for inp in sorted_args:
        arg = {}
        arg['name'] = inp['name']
        arg['type'] = PTYPE[inp['type']]
        if 'default' in inp:
            arg['type'] += ", optional"
        arg['description'] = inp['desc']
        function['docstring_arguments'].append(arg)

    function['return_arguments'] = []
    for ret in data['return']:
        arg = {}
        arg['name'] = ret['name']
        arg['type'] = PTYPE[ret['type']]
        arg['description'] = ret['desc']
        function['return_arguments'].append(arg)

    functions.append(function)

with open('src/MontagePy/wrappers.pxd', 'w') as f:
    f.write(template_pxd.render(functions=functions))

with open('src/MontagePy/_wrappers.pyx', 'w') as f:
    f.write(template_pyx.render(functions=functions))

with open('src/MontagePy/main.pyx', 'w') as f:
    f.write(template_main_pyx.render(functions=functions))
