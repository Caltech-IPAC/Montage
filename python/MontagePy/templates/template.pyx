# http://stackoverflow.com/questions/17014379/cython-cant-convert-python-object-to-double

from . cimport wrappers

cimport cpython.array as _array

{% for func in functions %}

cdef {{ func.name }}_cy({{ func.arguments_cdef|join(', ') }}):
    cdef wrappers.{{ func.name }}Return *ret
    ret = wrappers.{{ func.name }}({{ func.arguments|join(', ') }})
    retdict = {}
    
    if ret.status == 1:
        retdict['status'] = '1'
        retdict['msg'] = ret.msg
    
    else:
        {% for var in func.struct_vars %}retdict['{{ var }}'] = ret.{{ var }}
        {% endfor %}retdict['status'] = '0'
        del retdict['msg']
        
    return retdict

def {{ func.name }}({{ func.arguments_with_defaults|join(', ') }}):
    {% for line in func.array %}
    {{ line }}
    {% endfor %}
    return {{ func.name }}_cy({{ func.arguments_py_to_cdef|join(', ') }})

{% endfor %}
