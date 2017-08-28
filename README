This is a modified version of the PHP module php-kadm5, extended to
allow authentication using Kerberos credentials stored in a keytab file.

Changelog:

Updates to kadm5-0.2.3 by Ben Lasley (blasley@cs.stanford.edu) 11:41AM 7-23-2010

php_kadm5.h:48 	inserted php function declaration for kadm5_init_with_skey

kadm5.c:79		inserted php function declaration for kadm5_init_with_skey
kadm5.c:491-550	inserted php function kadm5_init_with_skey which mirrors kadm5_init_with_password, exchanging the password argument for a keytab argument and wrapping "kadm5/admin.h" function kadm5_init_with_password.


=======================



Well, the config.m4 file is still a mess. Unfortenately there is no
libkadm55-dev debian package yet. So you have to have the krb5 sources
at hand and change config.m4 accordingly.

To successfully compile this extension under Debian you need the
following Kerberos packages:

libkrb5
libkrb5-dev
libkadm55

The missing of libkadm55-dev has already been reported to the libkadm55
maintainer: http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=191616

If there are any questions or problems, feel free to contact me:

Holger Burbach <holger.burbach@gonicus.de>


