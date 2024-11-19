#!/bin/sh

rm std.out 2>/dev/null
rm -r hello_worldODBC.rdm  2>/dev/null

odbc01Example >std.out
stat=$?

if [ $stat -ne 0 ];then
    echo "Error running HelloWorldODBC tutorial"
fi
if [ `grep -c "Hello World" std.out ` -ne 1 ]; then
    echo "Expected output not found"
    stat=1
fi

rm std.out 2>/dev/null
rm -r hello_worldODBC.rdm  2>/dev/null
rm tfserver.lock 2>/dev/null

exit $stat
