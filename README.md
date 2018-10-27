libbthread
==========

library that provide some missing posix threading function to the bionic libc.

while i was porting tens of linux projects under the dSploit android app i found that
the bionic libc does not provide some POSIX thread functions like `pthread_cancel`, `pthread_testcancel` and so on.

so, i developed this library, which exploit some unused bits in the bionic thread structure.

there is many thing to develop, like support for deferred cancels, but basic thread cancellation works :smiley:

i hope that you find this library useful :wink: 

-- tux_mind

License
==========

Project is licensed under GNU LGPL v2.0 (Library General Public License)

pt-internal.h - is from The Android Open Source Project and licensed under Apache License, Version 2.0
