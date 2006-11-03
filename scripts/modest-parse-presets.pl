#! /usr/bin/perl -w

# Copyright (c) 2006, Nokia Corporation
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
# * Neither the name of the Nokia Corporation nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
# OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

#
# Perl script to converts an Excel spreadsheet (arg 1)
# with provider information in a GKeyFile readable format
# for use in modest (see modest-presets.[ch] for details]
 
# The input is assumed to be MS-style UTF16-LE, and
# output will be UTF-8. 
##
# Program requires Spreadsheet::ParseExcel, which does all
# the interesting stuff

# input columns are like this:
# 0: MCC (0 For Global Email)
# 1: Mailbox Name
# 2: EmailAddress
# 3: OutgoingMailServer
# 4: Secure smtp: (0 = no\, 1=yes)
# 5: IncomingMailServer
# 6: SendMessage (0=Immediately; 1=During next conn.)
# 7: SendCopyToSelf (0=No; 1=Yes)
# 8: MailboxType (0=POP3; 1=IMAP4)
# 9: Security (0=Off; 1=On(143/110); 2=On(993/995);)
#10: APOPSecureLogin (0=Off; 1=On)
#11: RetrieveAttachment (0=No; 1=Yes)
#12: RetrieveHeaders (0=All; 1-99=User defined)

# some of these are properties of user-settings; however,
# we are only interested in server settings, so some
# of the data (6,7,11,12) will be ignored for our output.

use strict;
use Spreadsheet::ParseExcel;
#use Unicode::String qw(utf8 utf16le);
#Unicode::String->stringify_as('utf8');

die "usage: xls2cvs <file.xls>\n" unless @ARGV == 1;

my $file = $ARGV[0];
die "'$file' is not a readable file\n" unless -r $file;
  

my $xl   = new Spreadsheet::ParseExcel;
my $data = $xl->Parse($file) or die "could not parse $file: $!";

my $sheet = $data->{Worksheet}[0];

my $now = `date`;
chomp $now;

print "# generated on $now from " . $data->{File} . "\n";
print "# keys and their meaning:\n";
print "# [MailboxName]: name of the provider (eg. Wanadoo, Gmail)\n" .
      "# MCC: Mobile Country Code (Netherlands=204, France=208, ...\n" . 
      "#                           globals like GMail don't have one)\n" .
      "# OutgoingMailServer: name of the smtp server (eg. smtp.foo.fi)\n" . 
      "# SecureSMTP: 'true' if there's secure SMTP\n" .
      "# IncomingMailServer\n" .
      "# MailboxType: 'pop' or 'imap'\n" .
      "# SMTPSecurity: 'true' if SMTP is secure\n" .
      "# APOPSecureLogin: 'true' if APOP is supported\n\n";

# ignore the first row
for (my $r = $sheet->{MinRow} + 1 ; defined $sheet->{MaxRow} && $r <= $sheet->{MaxRow}; ++$r) {

    my $cell;

    # legend:
    # 0: MCC (mobile country code, or 0 for Global)
    # 1: MailboxName
    # 2: EmailAddress
    # 3: OutgoingMailServer
    # 4: SecureSmtp
    # 5: IncomingMailServer
    # 6: SendMessage
    # 7:
    
    next unless ($sheet->{Cells}[$r][0] && $sheet->{Cells}[$r][0]->Value =~ /\d+/);

    # name -> required, unique
    $cell = $sheet->{Cells}[$r][1];
    next unless ($cell);
    print "[" . $cell->Value . "]\n";

    # MCC -> TODO: convert to normal country code
    $cell = $sheet->{Cells}[$r][0];
    if ($cell->Value > 0) {
	print "MCC=" . $cell->Value . "\n";
    }

    # address -> required, unique
    #$cell = $sheet->{Cells}[$r][2];
    #print "EmailAddress=" . $cell->Value . "\n";

    # OutgoingMailServer
    $cell = $sheet->{Cells}[$r][3];
    print "OutgoingMailServer=" . $cell->Value . "\n" if ($cell);

    # SecureSmtp?
    $cell = $sheet->{Cells}[$r][4];
    print "SecureSmtp=true\n"  if ($cell && $cell->Value == 1);
    
    # IncomingMailServer
    $cell = $sheet->{Cells}[$r][5];
    if ($cell) {
	print "IncomingMailServer=" . $cell->Value;
	
	my $type = $sheet->{Cells}[$r][8]->Value;
	my $sec =  $sheet->{Cells}[$r][9]->Value;
	
	if ($sec == 2) {
	    if ($type == 0) { print ":995";}
	    if ($type == 1) { print ":993";}
	}
	print "\n";
	print "IncomingSecurity=$sec\n";
    }
    
    # MailboxType
    $cell = $sheet->{Cells}[$r][8];
    if ($cell) {
	print "MailboxType=";
	if ($cell->Value == '0') {print "pop"} else {print "imap"};
	print "\n";
    }

    $cell = $sheet->{Cells}[$r][10];
    print "APOPSecureLogin=true\n"  if ($cell && $cell->Value == 1);
}

# the end
