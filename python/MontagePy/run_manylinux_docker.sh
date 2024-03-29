[exoadmin@vmexcalibur02 ~]$ cat manylinux_term.sh 
#!/bin/sh

docker run --interactive --tty --user 0 --volume `pwd`/Montage:/Montage pytorch/manylinux-cuda102 /bin/bash
[exoadmin@vmexcalibur02 ~]$ 

