// 
// ======================================================================
// Copyright (C) 2000-2003 Instrumental Software Technologies, Inc. (ISTI)
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. If modifications are performed to this code, please enter your own 
// copyright, name and organization after that of ISTI.
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in
// the documentation and/or other materials provided with the
// distribution.
// 3. All advertising materials mentioning features or use of this
// software must display the following acknowledgment:
// "This product includes software developed by Instrumental
// Software Technologies, Inc. (http://www.isti.com)"
// 4. If the software is provided with, or as part of a commercial
// product, or is used in other commercial software products the
// customer must be informed that "This product includes software
// developed by Instrumental Software Technologies, Inc.
// (http://www.isti.com)"
// 5. The names "Instrumental Software Technologies, Inc." and "ISTI"
// must not be used to endorse or promote products derived from
// this software without prior written permission. For written
// permission, please contact "info@isti.com".
// 6. Products derived from this software may not be called "ISTI"
// nor may "ISTI" appear in their names without prior written
// permission of Instrumental Software Technologies, Inc.
// 7. Redistributions of any form whatsoever must retain the following
// acknowledgment:
// "This product includes software developed by Instrumental
// Software Technologies, Inc. (http://www.isti.com/)."
// 8. Redistributions of source code, or portions of this source code,
// must retain the above copyright notice, this list of conditions
// and the following disclaimer.
// THIS SOFTWARE IS PROVIDED BY INSTRUMENTAL SOFTWARE
// TECHNOLOGIES, INC. "AS IS" AND ANY EXPRESSED OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED.  IN NO EVENT SHALL INSTRUMENTAL SOFTWARE TECHNOLOGIES,
// INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
// 

#ifndef _WINNT
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "q3302ew.h"
#include "config.h"
#include "options.h"


/* handle_opts() - handles any command line options.
	 	and sets the Progname extern to the
		base-name of the command.

	Returns: 
		TRUE if options are parsed okay.
		FALSE if there are bad or conflicting
		options.

*/

int handle_opts(int argc, char ** argv)  {

  if(argc != 2) {
    usage();
    q3302ew_die("Config file not specified\n");
  }
  if (readConfig(argv[1]) == -1) {
    usage();
    q3302ew_die("Too many q3302ew.d config problems\n");
  }
  return 1;
}

void usage() {
	fprintf(stderr, "%s: usage - Version %s\n", Q3302EW_NAME, Q3302EW_VERSION);
	fprintf(stderr, "%s ew_config_file\n", Q3302EW_NAME);
}
