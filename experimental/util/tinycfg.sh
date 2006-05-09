gconftool-2 -s /apps/tinymail/accounts/count -t int 2
gconftool-2 -s /apps/tinymail/accounts/1/type -t string transport
gconftool-2 -s /apps/tinymail/accounts/1/proto -t string smtp
gconftool-2 -s /apps/tinymail/accounts/1/user -t string $1
gconftool-2 -s /apps/tinymail/accounts/1/hostname -t string mail.kernelconcepts.de

gconftool-2 -s /apps/tinymail/accounts/0/type -t string store
gconftool-2 -s /apps/tinymail/accounts/0/proto -t string imap
gconftool-2 -s /apps/tinymail/accounts/0/user -t string $1
gconftool-2 -s /apps/tinymail/accounts/0/hostname -t string tasha.kc.de
