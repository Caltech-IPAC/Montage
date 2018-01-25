import MontagePy._wrappers as _wrappers

{% for func in functions %}

def {{ func.name }}({{ func.arguments_with_defaults|join(', ') }}):
    """
    {{func.summary}}

    Parameters
    ----------
    {% for arg in func.docstring_arguments %}{{ arg.name }} : {{ arg.type}}
        {{ arg.description}}
    {% endfor %}
    """
    return _wrappers.{{ func.name }}({{ func.arguments|join(', ') }})

{% endfor %}
