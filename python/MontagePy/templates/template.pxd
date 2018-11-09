cdef extern from "montage.h":

    {% for func in functions %}

    cdef struct {{ func.name }}Return:
        {% for declaration in func.struct_vars_decl %}{{ declaration }}
        {% endfor %}

    cdef {{ func.name }}Return *{{ func.name }}({{ func.arguments_cdef|join(', ') }})

    {% endfor %}
