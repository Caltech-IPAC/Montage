import os
import platform

print('\nPlatform Info\n-------------\n')

system  = platform.system()
release = platform.release()
version = platform.version()

print('platform.architecture():            ', platform.architecture())
print('platform.machine():                 ', platform.machine())
print('platform.node():                    ', platform.node())
print('platform.processor():               ', platform.processor())
print('platform.python_build():            ', platform.python_build())
print('platform.python_compiler():         ', platform.python_compiler())
print('platform.python_branch():           ', platform.python_branch())
print('platform.python_implementation():   ', platform.python_implementation())
print('platform.python_revision():         ', platform.python_revision())
print('platform.python_version():          ', platform.python_version())
print('platform.python_version_tuple():    ', platform.python_version_tuple())
print('platform.release():                 ', platform.release())
print('platform.system():                  ', platform.system())
print('platform.system_alias():            ', platform.system_alias(system, release, version))
print('platform.version():                 ', platform.version())
print('platform.uname():                   ', platform.uname())
print('platform.win32_ver():               ', platform.win32_ver())
print('platform.win32_edition():           ', platform.win32_edition())
print('platform.win32_is_iot():            ', platform.win32_is_iot())
print('platform.mac_ver():                 ', platform.mac_ver())
print('platform.libc_ver():                ', platform.libc_ver())

try:
    print('platform.freedesktop_os_release():  ', platform.freedesktop_os_release())
except:
    pass

print('')
print('os.uname().sysname:                 ', os.uname().sysname)
print('os.uname().nodename:                ', os.uname().nodename)
print('os.uname().release:                 ', os.uname().release)
print('os.uname().version:                 ', os.uname().version)
print('os.uname().machine:                 ', os.uname().machine)

print('')
