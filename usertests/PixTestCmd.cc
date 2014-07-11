// -- author: Wolfram Erdmann
// get analog current vs vana from testboard and roc readback

#include <algorithm>  // std::find
#include "PixTestCmd.hh"
#include "log.h"
#include "constants.h"
#include <TGLayout.h>
#include <TGClient.h>
#include <TGFrame.h>

using namespace std;
using namespace pxar;

ClassImp(PixTestCmd)

//------------------------------------------------------------------------------
PixTestCmd::PixTestCmd( PixSetup *a, std::string name )
: PixTest(a, name)
{
  PixTest::init();
  init();
}

//------------------------------------------------------------------------------
PixTestCmd::PixTestCmd() : PixTest()
{
}

//------------------------------------------------------------------------------
bool PixTestCmd::setParameter( string parName, string sval )
{
   if  (parName == sval) return true; // silence the compiler warnings
  return true;
}

//------------------------------------------------------------------------------
void PixTestCmd::init()
{
    LOG(logINFO) << "PixTestCmd::init()";
    fDirectory = gFile->GetDirectory( fName.c_str() );
    if( !fDirectory )
        fDirectory = gFile->mkdir( fName.c_str() );
    fDirectory->cd();
  
    
}

// ----------------------------------------------------------------------
void PixTestCmd::setToolTips()
{
  fTestTip = string( "void");
  fSummaryTip = string("summary plot to be implemented");
}

//------------------------------------------------------------------------------
void PixTestCmd::bookHist(string name)
{
  LOG(logDEBUG) << "nothing done with " << name;
}

//------------------------------------------------------------------------------
PixTestCmd::~PixTestCmd()
{
  LOG(logDEBUG) << "PixTestCmd dtor";
  std::list<TH1*>::iterator il;
  fDirectory->cd();
  for( il = fHistList.begin(); il != fHistList.end(); ++il ) {
    LOG(logINFO) << "Write out " << (*il)->GetName();
    (*il)->SetDirectory(fDirectory);
    (*il)->Write();
  }
  
}

void PixTestCmd::DoTextField(){
    string s=commandLine->GetText();
    transcript->SetForegroundColor(1);
    transcript->AddLine((">"+s).c_str());
    
    int stat = cmd->exec( s );
    string reply=cmd->out.str();
    if (reply.size()>0){
        if(stat==0){
            transcript->SetForegroundColor(0x0000ff);
        }else{
            transcript->SetForegroundColor(0xff0000);
        }
        transcript->AddLine( reply.c_str() );
    }
    if(transcript->ReturnLineCount() > 6)
        transcript->SetVsbPosition(transcript->ReturnLineCount());

    commandLine->SetText("");
    cmdHistory.push_back(s);
    historyIndex=cmdHistory.size();
}
void PixTestCmd::DoUpArrow(){
    if (historyIndex>0) historyIndex--;
    commandLine->SetText(cmdHistory[historyIndex].c_str());
}

void PixTestCmd::DoDnArrow(){
    if (historyIndex<cmdHistory.size()){
        historyIndex++;
    }
    if(historyIndex<cmdHistory.size()){
        commandLine->SetText(cmdHistory[historyIndex].c_str());  
    }else{
        commandLine->SetText("");
    }
}

void PixTestCmd::createWidgets(){
    const TGWindow *main = gClient->GetRoot();
    unsigned int w=600;
    tf = new TGTransientFrame(gClient->GetRoot(), main, 600, 600);//w,h
    
    // == Transcript ============================================================================================

    textOutputFrame = new TGHorizontalFrame(tf, w, 100);
    transcript = new TGTextView(textOutputFrame, w, 100);
    textOutputFrame->AddFrame(transcript, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 2, 2, 2, 2));

    tf->AddFrame(textOutputFrame, new TGLayoutHints(kLHintsExpandX| kLHintsExpandY, 10, 10, 2, 2));


    // == Command Line =============================================================================================

    cmdLineFrame = new TGHorizontalFrame(tf, w, 100);
    commandLine = new TGTextEntry(cmdLineFrame, new TGTextBuffer(60));
    commandLine->Connect("ReturnPressed()", "PixTestCmd", this, "DoTextField()");
    commandLine->Connect("CursorOutUp()", "PixTestCmd", this, "DoUpArrow()");
    commandLine->Connect("CursorOutDown()", "PixTestCmd", this, "DoDnArrow()");
    cmdLineFrame->AddFrame(commandLine, new TGLayoutHints(kLHintsExpandX, 2, 2, 2, 2));
    tf->AddFrame(cmdLineFrame, new TGLayoutHints(kLHintsExpandX, 10, 10, 2, 2));
    transcript->AddLine("Welcome to psi46expert!");
    tf->MapSubwindows();
    tf->Resize(tf->GetDefaultSize());
    tf->MapWindow();
}

//------------------------------------------------------------------------------

void PixTestCmd::doTest()
{
    LOG(logINFO) << "PixTestCmd::doTest() " ;

    fDirectory->cd();
    fHistList.clear();
    PixTest::update();

    createWidgets();
    cmd = new CmdProc();
    cmd->fApi=fApi;

    PixTest::update();
    //fDisplayedHist = find( fHistList.begin(), fHistList.end(), h1 );

}






/*====================================================================*/
/*  command processor code                                            */
/*====================================================================*/












#include <sstream>
#include <string>
#include <strings.h>  //index
#include <iomanip>


/**********************  Token handling with macros  ********/
string Token::front(bool expand){
    string t="";
    if (stack.empty()){ 
      t=token.front(); 
    }else{
      t=stack.back().front();
    }
    if (!expand) return t;
    mi = macros->find(t);
    if (mi==macros->end()) return t;
    token.pop_front(); // remove the macro token!
    stack.push_back(mi->second);
    return front();
}

void Token::pop_front(){
    if (stack.empty()){ 
      token.pop_front();
    }else{
      if(stack.back().empty()){cerr << "bug alert" << endl; return;}
      stack.back().pop_front();
      if (stack.back().empty()){stack.pop_back();};
    }
}
void Token::push_front(string s){
    if (stack.empty()){ 
      token.push_front(s);
    }else{
      stack.back().push_front(s);
    }
}

bool Token::assignment(string& name){
  // two-symbol-look-ahead kludge for assignments
  // returns the name in front of the assignment operator if there is one
  // otherwise restores the token queue as if nothing ever happened
  name = front(false);
  pop_front();
  if (!(empty())&& (front(false)=="=")){
    pop_front();
    return true;
  }else{
    //restore and return false
    push_front(name);
    return false;
  }
}

/***** generic parser tools, mostly recycled from psi46expert/syscmd *****/

bool StrToI(string word, int & v)
/* convert strings to integers. Numbers are assumed to
   be integers unless preceded by "0x", in which
   case they are taken to be hexadecimal,
   the return value is false if an error occured
*/

{
    const char * digits = "0123456789ABCDEF";
    int base;
    int i;
    const char * d0 = index(digits, '0');
    int len = word.size();
    /* $ indicate macros now
    if ((len>0) && (word[0] == '$')) { 
        base = 16;
        i = 1;
	} else*/ 
      if ((len>1) && (word[0] == '0') && (word[1] == 'x')) {
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

    v = a;
    if (i == len) {return true; } else { return false;}

}


bool parseIntegerList( Token & words,  vector<int> & ilist, bool append=false){
  /* parse a list of integers (rocs, rows, columns..), such as
     1:5,8:16  or 1,2,5,7
     hexadecimal values are allowed (see StrToI)
     recycled from syscmd
  */
  //cout << "parsing integer list starting with " << words.front() << endl;
  if (!append) ilist.clear();

  if ( (words.front()=="*") ){
    words.pop_front();
    for(int i=0; i<16; i++){ ilist.push_back(i);}
  }else{
    int i1,i2 ; // for ranges
    do
      {
	if (! StrToI(words.front(), i1)) {
	  //cerr << "illegal integer " << endl;
	  return false;
	}
	i2 = i1;
	words.pop_front();
	// optionally followed by :i2 to define a range
	if ( (!words.empty()) && (words.front()==":")){
	  words.pop_front();
	  if (! StrToI(words.front(), i2)) {
	    cerr << "illegal integer " << endl;
	    return false;
	  }
	  words.pop_front();
	}
	for (int j = i1; j <= i2; j++) { ilist.push_back(j);}

	// continue until no more comma separated list elements found
	if ( (!words.empty()) && (words.front()==",")){
	  words.pop_front();
	}else{
	  break;
	}
      }
    while (!words.empty());
    // end of the list reached
  }
  //cout << "returning ilist with size " << ilist.size() << endl;
  return true;
}


bool is_whitespace(unsigned int  i, const string & s){
  if ( i<s.size() ){
    return (s[i]==' ') || (s[i]=='\n')||(s[i]=='\t');
  }else{
    return false;
  }
}


string findchar(char c, unsigned int & i, string s){
  // starting from position i, find the next occurence of character 
  // c in in string s and return everything up-to (but excluding) c
  // as a string, afterwards
  // on exit, i points to c
  string word;
  while( i<s.size() && !(s[++i]==c)){
    word.push_back(s[i]);
  }
  return word;
}



deque <string > getWords(const string & s){
  // chop a string into words
  deque<string> words;
  unsigned int i=0;
  const char * symbols="\",:()[];=\n";

  string word="";
  while( i<s.size() ){

    while( is_whitespace(i,s) ){i++;};

    if (s[i]=='"'){
      word = findchar('"',i,s);
      if ((i<s.size()) && (s[i]=='"')) i++;
    }else  if ( index(symbols, s[i]) != NULL){
      word.push_back(s[i++]);
    }else{
      while( (i<s.size()) && (! is_whitespace(i,s)) 
	     && (index(symbols, s[i]) == NULL) ) {
	word.push_back(s[i++]);
      }	  
    }
    words.push_back(word);
    word="";
  }
  return words;
}

string Target::str(){
  stringstream s;
  s<<"["<<name;
  if (name=="roc"){
    for(unsigned int i=0; i<values.size()-1; i++){
      s<< " " << values[i] << ",";
    }
    s<<" "<<values[values.size()-1];
  }
  s<<"]";
  return  s.str();
}

/*********************** syntax element parsers ***********************/

bool Target::parse(Token & token){
  if (token.empty()) return false;
  name = token.front();
  if (name=="roc") {
    token.pop_front();
    if (!token.empty()){
      // is the next token a number or wildcard?
      string s= token.front();
      if ((s=="*") || (s==":") || isdigit( s[0] )){
	// FIXME wildcard list should be configurable
	return parseIntegerList(token, values); 
      }else{
	return false;	// FIXME could make sense for single rocs
      }
    }
    cerr << "error parsing roc ids" << endl;
    return false;
  }else if ((name=="tbm") || (name=="tb")) {
    token.pop_front();
    return true;
  }else{
    return false;
  }
  return false;
}


bool Statement::parse(Token & token){
    /*
        statement      :=    [ [<target>] [<keyword> <arguments>* | block] ]  
                 |  <name> "=" [value | block]
    */
    if (token.empty()) return false;

    if ( token.assignment( name ) ){
        isAssignment = true;
        if (token.front()=="["){

            // macro definition
            deque<string> macro;
            int n=0;
            do{
                string t=token.front();
                if(t=="["){ n++;} else if (t=="]"){ n--;}
                macro.push_back(t);
                token.pop_front();
            }while((n>0)&&(!token.empty()));

            if (n==0){
                token.add_macro(name, macro);
                return true;
            }else{
                cerr << "error in macro definition" << endl;
                return false;
            }
                     
        }else{
            block=NULL;
            //cout << "value list definition not implemented yet" <<endl;
            //return parseIntegerList(token, values); // dummy code
        }
        return false; // shouldn't get here
    }

    isAssignment=false;
    has_localTarget = localTarget.parse( token );


    if (token.empty()){
        // nothing follows
        block=NULL;
        keyword.keyword="";
    }else{

        // ... followed by keyword or block ?
        if( token.front()=="["){

            block = new Block(); 
            block->parse( token );

        }else if (!(token.front()==";")){

            // parse keyword + args
            block=NULL;
            keyword = Keyword( token.front() );
            token.pop_front();
            vector<int> ilist;
            while( (!token.empty()) &&  !( (token.front()==";") || (token.front()=="]") ) ){
                if (parseIntegerList(token, ilist)){
                    keyword.argv.push_back(ilist);
                }else{	   
                    keyword.argv.push_back(token.front());
                    token.pop_front();
                }
            }
            return true;

        }else{
            cerr << "syntax error?" << endl;
            return false;
        }
    }
    return true;
}



bool Block::parse(Token & token){
  /*
    block    :=  "[" <statement> ( ";" <statement> ) "]"
  */
  if (token.empty()) return false;
  //cout << "parsing Block starting with " << token.front() << endl;
  stmts.clear();
  if ( !(token.front()=="[") ) return false;
  token.pop_front();

  while( !token.empty() ){
    Statement * st = new Statement();
    bool stat = st->parse( token );
    if (! stat) return false;
    stmts.push_back( st );
    
    if (!token.empty()){
      if (token.front()=="]"){
	token.pop_front();
	break;
      } else if (token.front()==";"){
	token.pop_front();
      }else{
	cerr << "expected ';' instead of " << token.front() << endl;
	return false;
      }
    }

  }
  
  return true; 

}


/*********************** keyword decoding helpers**********************/


bool  Keyword::match(const char * s, int & value){
  return  (kw(s)) && (narg()==1) && (argv[0].getInt(value));
}

bool  Keyword::match(const char * s1, string & s2){
  return  (kw(s1)) && (narg()==1) && (argv[0].getString(s2));
}

bool Keyword::match(const char * s, vector<int> & v1, vector<int> & v2){
  return  (kw(s)) && (narg()==2) && (argv[0].getList(v1)) && (argv[1].getList(v2));
}

string Keyword::str(){
  stringstream s;
  s << keyword << "(";
  for(unsigned int i=0; i<narg(); i++){
    s << argv[i].str();
    if (i+1<narg()) s <<",";
  }
  s << ")";
  return s.str();  
}
/******************  execution / processing **************************/


bool Block::exec(CmdProc * proc, Target & target){
  //cout << "executing block [" << statements.size() << "]" << endl;
  bool success=true;
  for (unsigned int i=0; i<stmts.size(); i++){
    success |= stmts[i]->exec(proc, target); 
  }
  return success;
}



bool Statement::exec(CmdProc * proc, Target & target){

  if (isAssignment){
    //proc->setVariable( targets.name, block );
    //cout << "assignment" <<endl;
    return true; 

  }else{

    if( has_localTarget && (block==NULL) && (keyword.keyword=="") ){
      // just a target definition, set the default
      proc->setDefaultTarget( localTarget );
      proc->out << "target set to " << proc->defaultTarget.str();
      return true;
    }

    Target useTarget;
    if( has_localTarget ){
      useTarget = localTarget;
    }else{
      useTarget = target; 
    }
	  
      
    if ( (block==NULL) && !(keyword.keyword=="") ){

    // keyword + arguments ==> execute
      for(unsigned int i=0; i<useTarget.values.size();  i++){
	Target t = useTarget.get(i);
	if (! (proc->process(keyword,  t))) return false;
      }
      return true;

    }else if (!(block==NULL) ){

      for(unsigned int i=0; i< useTarget.values.size();  i++){
	Target t = useTarget.get(i);
	if (!(block->exec(proc, t) )) return false;
      }
      return true;

    }else{
      
      return false;
      // should not be here

    }

  }
  return false;// should not get here
}



/* Command Processor */
CmdProc::CmdProc()
{
  verbose=false;
  defaultTarget = Target("tb");
  //fApi = new DummyApiJustForTesting();
}



/**************** call-backs for script processing ***********************/

int CmdProc::tb(Keyword kw){
    /* implementation of testboard commands */
    string s;
    if ( kw.match("ia") ){  out <<  "ia=" << fApi->getTBia(); }
    else if( kw.match("hvon")  ){ fApi->HVon(); }
    else if( kw.match("hvoff") ){ fApi->HVoff(); }
    else if( kw.match("pon")   ){ fApi->Pon(); }
    else if( kw.match("poff")  ){ fApi->Poff(); }
    else if( kw.match("adc")   ){
        fApi->daqStart();
        fApi->daqTrigger(1);
        std::vector<pxar::Event> buf = fApi->daqGetEventBuffer();
        fApi->daqStop();
        for(unsigned int i=0; i<buf.size(); i++){
            out  << buf[i];
        }
    }
    else if( kw.match("adcraw")   ){
        fApi->daqStart();
        fApi->daqTrigger(1);
        std::vector<uint16_t> buf = fApi->daqGetBuffer();
        fApi->daqStop();
        for(unsigned int i=0; i<buf.size(); i++){
            out << " " <<hex << setw(4)<< setfill('0')  << buf[i];
        }
    }
    else{
        out << "unknown testboard command " << kw.str();
        return 1;
    }
    return 0;
}


int CmdProc::roc( Keyword kw, int rocId){
    /* implementation of roc commands */
    vector<int> col, row;
    int value;
    if (kw.match("vana", value))  { fApi->setDAC("Vana",value, rocId ); }
    else if (kw.match("hirange")) {fApi->setDAC("ctrlreg", fApi->_dut->getDAC(rocId, "ctrlreg")|4, rocId);}
    else if (kw.match("lorange")) {fApi->setDAC("ctrlreg", fApi->_dut->getDAC(rocId, "ctrlreg")&0xfb, rocId);}
    else if (kw.match("enable") ) {fApi->setDAC("ctrlreg", fApi->_dut->getDAC(rocId, "ctrlreg")|2, rocId);}
    else if (kw.match("disable")) {fApi->setDAC("ctrlreg", fApi->_dut->getDAC(rocId, "ctrlreg")&0xfd, rocId);}
    else if (kw.match("mask")){ fApi->_dut->maskAllPixels(true, rocId);}
    else if (kw.match("arm", col, row)||(kw.match("pixd", col, row))
      ||kw.match("pixe", col, row)
    ){
        for(unsigned int c=0; c<col.size(); c++){
            for(unsigned int r=0; r<row.size(); r++){
                if (kw.keyword=="arm"){
                    fApi->_dut->testPixel(col[c], row[c], true); // which roc?
                    fApi->_dut->maskPixel(col[c], row[c], false);
                }else if (kw.keyword=="pixd"){
                    fApi->_dut->testPixel(col[c], row[c], true);
                    fApi->_dut->maskPixel(col[c], row[c], true);
                }else if (kw.keyword=="pixe"){
                    fApi->_dut->testPixel(col[c], row[c], true);
                    fApi->_dut->maskPixel(col[c], row[c], false);
                }
             }
        }
        out << "this doesn't really work, I don't seem to understand the api";
        /*
          for f in `find . -name *.h*`; do sed -i -e "s/private\:/public\:/" $f; done
        fApi->MaskAndTrim(true);
        fApi->SetCalibrateBits(true);
        * */
    }else{
        out << "unknown roc command " << kw.str();
        return 1;
    }
    
    
  return 0;
}



bool CmdProc::process(Keyword keyword, Target target){
    // this is where things are actually done (eventually)

    // debugging printout
    if (verbose){
        cout << "keyword  -->  " << keyword.str() << " " << target.str() << endl;   
    }


    if( keyword.match("macros")){
    // dump the list of macros
        for( map<string, deque<string> >::iterator it=macros.begin(); 
            it!=macros.end(); it++){
            out << it->first << ":";
            for( unsigned int i=0; i<it->second.size(); i++){
                out << it->second[i] << " ";
            }
            out << endl;
        }
        return true;
    }
    if (keyword.match("help")){
        out << "under construction, essentially compatible to the psi46expert command line";
        return true;
    }



    // 
    if(target.name=="tb"){

        tb( keyword );

    } else if (target.name=="tbm") {
        
        // todo
        
    } else if (target.name=="roc") {

        roc(keyword, target.value());

    }

  return true;
} 





/* driver */
int CmdProc::exec(std::string s){
    
    out.str("");
    // parse and execute a string, leads to call-backs to CmdProc::process
    Token words( getWords(s) );
    words.macros=&macros;
    macros["test"] = getWords("roc 0 vana 5");

    int j=0;
    //parse
    vector < Statement > stmts;
    while( (!words.empty()) && (j++ < 2000)){
        Statement c;
        bool stat=  c.parse( words );
        if( stat){
            stmts.push_back( c );
        }
        if (!words.empty()) {
            if (words.front()==";") {
                words.pop_front();
            }else{
                out << "expected ';' instead of " << words.front();
                return 1;
            }
        }
    }

    bool ok = true;
    for( unsigned int i=0; i<stmts.size(); i++){
        ok |= stmts[i].exec(this, defaultTarget);
    }

    if (ok){
        return 0;
    }else{
        return 2;
    }
}





