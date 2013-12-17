/*************************************************************
 *
 *    SysCommand
 *
 *  class representing system commands
 *  a system consists of multiple modules, with tbms and rocs
 *
 *  A single command is contained in the following public fields
  int type;                 // what kind of target kTB,kTBM,kROC
  int module;               // module id
  int roc;                  // roc id (when type=kROC)
  int narg;                 // number of arguments
  char* carg[nArgMax];      // keyword or NULL (`\0` terminated)
  int*  iarg[nArgMax];      // integer list    (-1 terminated)
  *
  *
  *
  *************************************************************/

#include "SysCommand.hh"
#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include <iostream>
#include <fstream>
#include <cctype>
using namespace std;


//-----------------------------------------------------------

SysCommand::SysCommand() {
  defaultTarget.type = kROC;
  defaultTarget.nCN = 1;
  defaultTarget.CN[0] = 0;
  defaultTarget.nModule = 1;
  defaultTarget.hub[0] = 0;
  defaultTarget.nRoc = 1;
  defaultTarget.id[0] = 0;
  narg = 0;
  nOpen = 0;
  inputFile = NULL;
}

//-----------------------------------------------------------
char * SysCommand::GetWord(char * s, int * l) {
  /*
    helper for chopping a line into tokens, a variation of strtok,
    with two differences:
    1) no '\0' characters are inserted, instead the length
    of the string is returned
    2) the list of separators is hard-coded (sym/white)

    return values:
    * pointer to the 1st character of the Next word (or symbol)
    * number of characters in that word,l
    l>0   : length of the word starting at the pointer returned
    l==0  : end of line reached, no new symbol
  */

  static int idx;
  static char * buf;        // pointer to the input line (not a copy!!)
  const char * sym = ",:()"; // single character symbols
  const char * white = " \t\n"; // whitespace characters
  char * word;
  int maxLen = 40;

  // first call with a new string,
  if (s != NULL)
    {
      idx = 0; buf = s;
    }
  // skip whitespace characters
  while ((buf[idx] != '\0') && (index(white, buf[idx]) != NULL)) {idx++; }
  // set the pointer to the beginning of the Next word/symbol
  word = &buf[idx];
  if (buf[idx] == '\0')
    {
      word = NULL;
      *l = 0; // the end
    }
  else if (index(sym, buf[idx]) != NULL)
    {
      // single letter symbols, return them as words
      *l = 1; idx++;
    }
  else
    {
      // extract words delimited by symbols or blanks
      *l = 0;
      while ((buf[idx] != '\0') && (*l < maxLen)
	     && (index(sym, buf[idx]) == NULL)
	     && (index(white, buf[idx]) == NULL))
        {
	  idx++; (*l)++;
        }
    }
  return word;
}


// ----------------------------------------------------------------------
bool SysCommand::IncludesRoc(int rocID) {
  for (int i = 0; i < target.nRoc; i++)
    {
      if (target.id[i] == rocID) return true;
    }
  return false;
}


//------------------------------------------------------------
/* convert strings to integers. Numbers are assumed to
   be integers unless preceded by "$" or "0x", in which
   case they are taken to be hexadecimal,
   the return value is non-zero if an error occured
*/
int SysCommand::StrToI(const char * word, const int len, int * v) {
  const char * digits = "0123456789ABCDEF";
  int base;
  int i;
  const char * d0 = index(digits, '0');
  if (word[0] == '$') {
    base = 16;
    i = 1;
  } else if ((word[0] == '0') && (word[1] == 'x')) {
    base = 16;
    i = 2;
  } else {
    base = 10;
    i = 0;
  }

  int a = 0;
  const char * d;
  while ((i < len) && ((d = index(digits, word[i])) != NULL)) {
    a = base * a + d - d0;
    i++;
  }

  *v = a;
  if (i == len) {return 0; } else { return 1;}

}

//------------------------------------------------------------
int SysCommand::Parse(char * line) {
  // Parse one line of instruction
  // the return value is non-zero when the result is an
  // executable instruction

  narg = 0;
  // avoid crashed for code that doesnt check for narg>0
  carg[0] = &cbuf[0]; cbuf[0] = '\0';
  exitFlag = false;
  // skip comments and empty lines
  if (line[0] == '#')  return 0; /* filter out the comments */
  if (line[0] == '\n') return 0; /* filter out empty lines */
  if (line[0] == 0)    return 0; /* filter out empty lines */

  char * carg0[nArgMax];
  int * iarg0[nArgMax];
  int iterate[nArgMax];

  int kbuf = 0;   // index of Next integer in ibuf
  int kcbuf = 0;  // index to Next character in cbuf
  int narg0 = 0;
  int l;
  char * word;

  int i1, i2;

  //    Part I: chop the input into words and Parse integer lists
  word = GetWord(line, &l);
  while ((l > 0) && (narg0 < nArgMax))
    {

      if (index("$0123456789", word[0]) != NULL)
        {
	  //                         ------  integer value, range or list  ------
	  iarg0[narg0] = &ibuf[kbuf];
	  carg0[narg0] = NULL;
	  iterate[narg0] = 0;
	  do
            {
	      if (StrToI(word, l, &i1)) {
		cerr << "illegal integer " << endl;
		narg = 0; return 0;
	      }
	      i2 = i1;
	      word = GetWord(NULL, &l);
	      // optionally followed by :i2 to define a range
	      if ((l > 0) && (strncmp(word, ":", l) == 0))
                {
		  word = GetWord(NULL, &l);
		  if (word == 0)
                    {
		      cerr << "invalid range" << endl;
		      narg = 0;
		      return 0;
                    }
		  //sscanf(word,"%d",&i2);
		  if (StrToI(word, l, &i2)) {
		    cerr << "illegal integer " << endl;
		    narg = 0; return 0;
		  }
		  word = GetWord(NULL, &l);
                }
	      for (int j = i1; j <= i2; j++)
                {
		  ibuf[kbuf++] = j;
                }
	      // continue until no more comma separated list elements found
	      if ((l > 0) && (strncmp(word, ",", l) != 0))
                {
		  break;
                }
	      else
                {
		  word = GetWord(NULL, &l);
                }
            }
	  while ((l > 0) && (kbuf < 999));
	  // end of the list reached
	  ibuf[kbuf++] = -1;

        }
      else if ((l > 0) && (strncmp(word, "(", l) == 0))
        {   // ------  iterated list  ------

	  iarg0[narg0] = &ibuf[kbuf];
	  carg0[narg0] = NULL;
	  iterate[narg0] = 0;

	  word = GetWord(NULL, &l); // swallow bracket
	  while (strncmp(word, ")", l) != 0)
            {
	      //sscanf(word,"%d",&i1); i2=i1;
	      if (StrToI(word, l, &i1)) {
		cerr << "illegal integer " << endl;
		narg = 0; return 0;
	      }
	      i2 = i1;
	      word = GetWord(NULL, &l);
	      // optionally followed by :i2 to define a range
	      if (strncmp(word, ":", l) == 0)
                {
		  word = GetWord(NULL, &l); // get i2
		  if (l == 0) {
		    cerr << "invalid range" << endl;
		    narg = 0; return 0;
		  }
		  if (StrToI(word, l, &i2)) {
		    cerr << "illegal integer " << endl;
		    narg = 0; return 0;
		  }
		  word = GetWord(NULL, &l); // move on
                }
	      for (int j = i1; j <= i2; j++)
                {
		  ibuf[kbuf++] = j; // put range into list
		  iterate[narg0]++; // count entries
                }
	      // ignore commas
	      if (strncmp(word, ",", l) == 0) { word = GetWord(NULL, &l);}
            }
	  // end of the list reached
	  word = GetWord(NULL, &l); // move past ")"
	  ibuf[kbuf++] = -1;
        }
      else
        {   //                         ------  keyword  ------

	  iarg0[narg0] = NULL;
	  carg0[narg0] = &cbuf[kcbuf];
	  iterate[narg0] = 0;

	  strncpy(carg0[narg0], word, l);
	  kcbuf += l;
	  carg0[narg0][l] = '\0';
	  kcbuf++;
	  word = GetWord(NULL, &l);
        }
      narg0++;
    }

  //    debugging
  /*
    cout << "{{";
    for(int i=0; i<narg0; i++){
    if(carg0[i]==NULL) {
    for(int* j=iarg0[i]; (*j)>=0; j++){
    cout << *j  << " ";
    }
    }else{
    cout << carg0[i] << " ";
    }
    }
    cout << "}}" << endl;
  */

  //    Part II  Parse target identifiers and fill visible data fields
  //

  int consumed = 0;
  // look for commands for the Parser itself
  if (strcmp(carg0[0], "verbose") == 0)
    {
      verbose = 1;
      return 0;
    }
  else if (strcmp(carg0[0], "quiet") == 0)
    {
      verbose = 0;
      return 0;
    }
  else if (strcmp(carg0[0], "exec") == 0)
    {
      if ((narg0 == 2) && (carg0[1] != NULL)) {
	// this leads to a recursive call of Parse
	if (Read(carg0[1]) == 0)
	  {
	    return 1;
	  } else {
	  return 0;
	}
      }
      else
        {
	  cerr << "exec filename??" << endl;
	  return 0;
        }
    }
  else if (strcmp(carg0[0], "echo") == 0)
    {
      for (int i = 1; i < narg0; i++) {
	if (carg0[i] != NULL) cout << carg0[i] << " ";
      }
      cout << endl;
      return 0;
    }
  else if ((strcmp(carg0[0], "exit") == 0) || (strcmp(carg0[0], "quit") == 0) || (strcmp(carg0[0], "q") == 0))
    {
      // stop reading
      exitFlag = true;
      return 0;
    }

  // not a Parser command

  // Next is the optional control network identifier "cn"
  if (strcmp(carg0[0], "cn") == 0)
    {
      consumed++;
      // always expand target lists
      if ((narg0 > consumed) && (iarg0[consumed] != NULL))
        {
	  // control network id(s) follow
	  target.nCN = 0;
	  for (int * j = iarg0[consumed]; ((*j) >= 0) && (target.nModule < nModuleMax); j++)
            {
	      target.CN[target.nCN++] = *j;
            }
	  consumed++;
        }
      else
        {
	  // no id given
	  cerr << "Warning! Control Network id expected after keyword cn" << endl;
	  // keep default anyway
	  target.nCN = defaultTarget.nCN;
	  for (int i = 0; i < target.nCN; i++) target.CN[i] = defaultTarget.CN[i];
        }
    }
  else
    {   // control network keyword absent, use default
      target.nCN = defaultTarget.nCN;
      for (int i = 0; i < target.nCN; i++) target.CN[i] = defaultTarget.CN[i];
    }


  // Next is the optional module identifier
  if (strcmp(carg0[0], "module") == 0)
    {
      consumed++;
      // always expand target lists
      if ((narg0 > consumed) && (iarg0[consumed] != NULL))
        {
	  // module id(s) follow
	  target.nModule = 0;
	  for (int * j = iarg0[consumed]; ((*j) >= 0) && (target.nModule < nModuleMax); j++)
            {
	      target.hub[target.nModule++] = *j;
            }
	  consumed++;
        }
      else
        {
	  // no id given
	  cerr << "Warning! Module id expected after keyword module" << endl;
	  // keep default anyway
	  target.nModule = defaultTarget.nModule;
	  for (int i = 0; i < target.nModule; i++) target.hub[i] = defaultTarget.hub[i];
        }
    }
  else
    {   // module keyword absent, use default
      target.nModule = defaultTarget.nModule;
      for (int i = 0; i < target.nModule; i++) target.hub[i] = defaultTarget.hub[i];
    }



  // optional target identifier test,tb,tbm, roc or sys
  if ((narg0 > consumed) && (strcmp(carg0[consumed], "test") == 0))
    {
      target.type = kTest;
      consumed++;
    }
  else if ((narg0 > consumed) && (strcmp(carg0[consumed], "tbm") == 0))
    {
      target.type = kTBM;
      target.nRoc = 0;
      consumed++;
    }
  else if ((narg0 > consumed) && (strcmp(carg0[consumed], "roc") == 0))
    {
      target.type = kROC;
      consumed++;
      // always expand target lists
      if ((narg0 > consumed) && (iarg0[consumed] != NULL))
        {
	  // rod id(s) follow
	  target.nRoc = 0;
	  for (int * j = iarg0[consumed]; ((*j) >= 0) && (target.nRoc < nRocMax); j++)
            {
	      target.id[target.nRoc++] = *j;
            }
	  consumed++;
        }
      else
        {
	  // no roc id given, assume 0, single roc
	  target.nRoc = 1; // unless specified otherwise
	  target.id[0] = 0;
        }
    }
  else if ((narg0 > consumed) && (strcmp(carg0[consumed], "tb") == 0))
    {
      target.type = kTB;
      target.nRoc = 0;
      consumed++;
    }
  else if ((narg0 > consumed) && (strcmp(carg0[consumed], "sys") == 0))
    {
      target.type = kSYS;
      target.nRoc = 0;
      consumed++;
    }
  else
    {   // no target is specified, imply the default
      target.type = defaultTarget.type;
      target.nRoc = defaultTarget.nRoc;
      for (int i = 0; i < target.nRoc; i++) target.id[i] = defaultTarget.id[i];

    }


  // any commands following?
  if (consumed == narg0)
    {
      // no more commands, just a new default target definition
      narg = 0;
      defaultTarget.type = target.type;
      defaultTarget.nCN = target.nCN;
      for (int i = 0; i < target.nCN; i++) defaultTarget.CN[i] = target.CN[i];
      defaultTarget.nRoc = target.nRoc;
      for (int i = 0; i < target.nRoc; i++) defaultTarget.id[i] = target.id[i];
      defaultTarget.nModule = target.nModule;
      for (int i = 0; i < target.nModule; i++) defaultTarget.hub[i] = target.hub[i];
      return 0;  // nothing else to be done
    }
  else
    {
      // more commands follow, fill visible argument list
      // target
      iHub = 0;
      iRoc = 0;
      type =   target.type;
      module = target.hub[0];
      roc =    target.id[0];
      // arguments
      narg = narg0 - consumed;
      for (int j = 0; j < narg; j++)
        {
	  iarg[j] = iarg0[j + consumed];
	  carg[j] = carg0[j + consumed];
	  if (iterate[j + consumed] > 0)
            {
	      if (iterate[j + consumed] == target.nRoc * target.nModule)
                {
		  // for iterated list arguments, throw in an extra list terminator
		  // to hide the rest of the list from the user
		  isIterated[j] = 1;
		  temp[j] = *(iarg[j] + 1);
		  *(iarg[j] + 1) = -1;
                }
	      else
                {
		  cerr << "length of iterated list doesn't match" << endl;
		  narg = 0;
		  return 0;
                }

            }
	  else
            {
	      // ordinary list argument
	      isIterated[j] = 0;
	      temp[j] = -1;
            }
        }
      return 1;  // do something
    }// non-empty command list

}

//------------------------------------------------------------
/* load the next simple command into the user accessible area.
   for single lines this implements iterations over lists
   when reading commands from a file it will also
   get the next line until the end of file is encountered

   returns 1 if successful, executable command has been loaded
   returns 0 if no more commands/iterations are available

*/
int SysCommand::Next() {
  if ((narg > 0) && (iHub < target.nModule)
      && ((iHub < target.nModule - 1) || (iRoc < target.nRoc - 1)))
    {

      // incrementmodule/roc id
      iRoc++;
      if (iRoc >= target.nRoc)
        {
	  iRoc = 0; iHub++;
        }

      // update visible addresses
      module = target.hub[iHub];
      roc   = target.id[iRoc];

      // advance pointers for list arguments
      for (int j = 0; j < narg; j++)
        {
	  if (isIterated[j])
            {
	      iarg[j]++;
	      *(iarg[j]) = temp[j];
	      temp[j] = *(iarg[j] + 1); // no overrun, -1 always follows
	      *(iarg[j] + 1) = -1;
            }
        }
      return 1;  // i.e. execute this

    }
  else
    {   // no more targets from previously parsed line
        // are we reading from a file ?

      if (nOpen > 0) {
	char line[100];
	// skip lines not meant to be executed
	while ((Getline(line, 99) == 0) && (Parse(line) == 0));
	return nOpen > 0;
      }
      else
        {   // no file, nothing else to do
	  return 0;
        }
    }

}


//------------------------------------------------------------
void SysCommand::Print() {
  do
    {
      switch (type)
        {
        case kTB:
	  cout << "TB:";
	  break;
        case kTBM:
	  cout << "Module " << module << " TBM:";
	  break;
        case kROC:
	  cout << "Module " << module <<  " ROC " << roc << ":";
	  break;
        }

      for (int i = 0; i < narg; i++)
        {
	  if (iarg[i] != NULL)
            {
	      cout << " " << *(iarg[i]);
	      for (int * j = iarg[i] + 1; (*j) >= 0; j++)
                {
		  cout << "," << *j ;
                }
            }
	  else
            {
	      cout << " " << carg[i];
            }
        }
      cout << endl;

    }
  while (Next());
}


//------------------------------------------------------------
char * SysCommand::toString() {
  const int bufsize = 1000;
  char * buf = new char[bufsize];
  buf[0] = '\0';

  switch (type)
    {
    case kTest:
      sprintf(buf, "");
      break;
    case kTB:
      sprintf(buf, "TB:");
      break;
    case kTBM:
      //sprintf(buf,"Module %d  TBM:",module);
      sprintf(buf, "TBM:");
      break;
    case kROC:
      //      sprintf(buf,"Module %d  ROC %d:",module,roc);
      sprintf(buf, "ROC %d:", roc);
      break;
    }

  for (int i = 0; i < narg; i++)
    {
      if (iarg[i] != NULL)
        {
	  int room = bufsize - strlen(buf);
	  snprintf(&buf[strlen(buf)], room, " %d", *(iarg[i]));
	  for (int * j = iarg[i] + 1; (*j) >= 0; j++)
            {
	      room = bufsize - strlen(buf);
	      snprintf(&buf[strlen(buf)], room, ",%d", *j);
            }
        }
      else
        {
	  int room = bufsize - strlen(buf);
	  snprintf(&buf[strlen(buf)], room, "%s", carg[i]);
        }
    }
  return buf;
}

//------------------------------------------------------------
void SysCommand::RocsDone() {
  iRoc = target.nRoc;
}

//------------------------------------------------------------
int SysCommand::Getline(char * line, int n) {
  while (!(inputFile->good()) && (nOpen > 0)) {
    delete inputFile;
    nOpen--;
    if (nOpen > 0) {
      inputFile = fileStack[nOpen - 1];
    } else {
      //      inputFile == NULL;
      inputFile = NULL;
      return 1;
    }
  }
  if (nOpen > 0) {
    inputFile->getline(line, n);
    return 0;
  } else {
    return 1;
  }

}

//------------------------------------------------------------
int SysCommand::Read(const char * fileName) {

  // open a new file
  if (nOpen < nFileMax) {
    inputFile = new ifstream(fileName);
    if (inputFile->is_open())
      {
	fileStack[nOpen++] = inputFile;
	char line[100];
	inputFile->getline(line, 99);
	Parse(line);  // this may be a recursive call
	return 0;
      }
    else
      {
	cerr << " Unable to open file " << endl;
	delete inputFile;
	inputFile = nOpen > 0 ? fileStack[nOpen - 1] : NULL;
	return 1;
      }
  } else {
    cerr << "max number of open files exceeded " << endl;
    return 1;
  }
}

//------------------------------------------------------------
bool SysCommand::Exit() {
  return exitFlag;
}

//------------------------------------------------------------
bool SysCommand::TargetIsTest() {
  return (type == kTest);
}

//------------------------------------------------------------
bool SysCommand::TargetIsTB() {
  return (type == kTB);
}


//------------------------------------------------------------
bool SysCommand::TargetIsTBM() {
  return (type == kTBM);
}

//------------------------------------------------------------
bool SysCommand::TargetIsROC() {
  return (type == kROC);
}



//------------------------------------------------------------
int SysCommand::PrintList(char * buf, int n, int v[]) {
  int a, b;
  int i = 0;
  int l = 0;

  while ((i < n) && (l < 90)) {
    a = b = v[i++];
    while ((i < n) && (v[i] == (b + 1))) {b++; i++;}
    if (a == b) {
      l += sprintf(&buf[l], "%d", a);
    } else {
      l += sprintf(&buf[l], "%d:%d", a, b);
    }
    if (i < n) l += sprintf(&buf[l], ",");
  }
  return l;
}

//-----------------------------------------------------------
int SysCommand::PrintTarget(char * buf, target_t * t, int mode) {
  // returns a string representation of the current target
  // mode  is a mask to enable individual components
  // bit 0 = module id
  // bit 1 = control network id
  int l = 0;

  if (mode & 0x2) {
    // prepend control network
    l += PrintList(&buf[l], t->nCN, t->CN);
    l += sprintf(&buf[l], ":");
  }

  if (t->type == kTB) {
    l += sprintf(&buf[l], "%s", "tb");
  } else {
    if (mode & 0x01) {
      // add module id
      l += sprintf(&buf[l], "[");
      l += PrintList(&buf[l], t->nModule, t->hub);
      l += sprintf(&buf[l], "]");
    }
    // always give TBM and roc
    if (t->type == kTBM) {
      l += sprintf(&buf[l], "tbm");
    } else if (t->type == kROC) {
      l += sprintf(&buf[l], "roc[");
      l += PrintList(&buf[l], t->nRoc, t->id);
      l += sprintf(&buf[l], "]");
    }
  }
  return l;
}

//-----------------------------------------------------------
char * SysCommand::TargetPrompt(int mode, const char * sep) {
  // returns a string created by printTarget and appends some separator
  static char buf[100];
  int l = PrintTarget(buf, &defaultTarget, mode);
  if (sep != NULL) { l += sprintf(&buf[l], "%s", sep);}
  return buf;
}

//-----------------------------------------------------------
int SysCommand:: GetTargetRoc(int * pModule, int * pRoc) {
  // provide current default target roc/module
  // return value is zero if the Target is a Roc,
  // nonzeror otherwise
  if (defaultTarget.type == kROC) {
    (*pModule) = defaultTarget.hub[0];
    (*pRoc) = defaultTarget.id[0];
    return 0;
  } else {
    return 1;
  }

}

//-----------------------------------------------------------
/* helper functions for analyzing commands of the type
   keyword value1 value2 ...
   in Execute methods.
   Keyword(...) returns true iff the keyword and the number of
   integer arguments match.
   On return the integer pointers point to the values in
   the commands internal buffer.
*/

bool SysCommand::Keyword(const char * keyword) {

  if ((carg[0] != NULL) && (strcmp(carg[0], keyword) == 0)
      && (narg == 1))
    {
      return true;
    } else {
    return false;
  }
}

bool SysCommand::Keyword(const char * keyword, int ** value) {
  if ((carg[0] != NULL) && (strcmp(carg[0], keyword) == 0)
      && (narg == 2) && (iarg[1] != NULL) && (*(iarg[1]) > -1))
    {
      *value = iarg[1];
      return true;
    } else {
    return false;
  }
}


bool SysCommand::Keyword(const char * keyword, int ** value1, int ** value2) {
  if ((carg[0] != NULL) && (strcmp(carg[0], keyword) == 0)
      && (narg == 3)
      && (iarg[1] != NULL) && (*(iarg[1]) > -1)
      && (iarg[2] != NULL) && (*(iarg[2]) > -1))
    {
      *value1 = iarg[1];
      *value2 = iarg[2];
      return true;
    } else {
    return false;
  }
}

bool SysCommand::Keyword(const char * keyword, const char * keyword1) {
  if ((carg[0] != NULL) && (strcmp(carg[0], keyword) == 0)
      && (narg == 2)
      && (carg[1] != NULL) && (strcmp(carg[1], keyword1) == 0))
    {
      return true;
    } else {
    return false;
  }
}

