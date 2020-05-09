#!/bin/sh
i=1
while [ ${i} -le 80 ]
do
if [ ${i} -le 9 ]; then
username=stu0${i}
else
username=stu${i}
fi
rm -r /home/${username}
userdel ${username}
i=$((${i}+1))
done
groupdel class1
exit 0

