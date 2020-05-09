#!/bin/sh
i=1
groupadd class1
while [ ${i} -le 80 ]
do
if [ ${i} -le 9 ]; then
username=stu0${i}
else
username=stu${i}
fi
useradd -g class1 ${username}
mkdir /home/${username}
chown -R ${username} /home/${username}
chgrp class1 /home/${username}
i=$((${i}+1))
done
exit 0

