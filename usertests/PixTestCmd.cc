// -- author: Wolfram Erdmann
// opens up a command line window

#include "PixTestCmd.hh"
#include "log.h"
#include "constants.h"
#include <TGLayout.h>
#include <TGClient.h>
#include <TGFrame.h>

#include <iostream>
#include <fstream>

//#define DEBUG

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
  fSummaryTip = string("nothing to summarize here");
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
  
  // dump the history file for future use
  ofstream fout(".history");
  for(unsigned int i=max(0,(int)cmdHistory.size()-100); i<cmdHistory.size(); i++){
      fout << cmdHistory[i] << endl;
  }
  
}

void PixTestCmd::DoTextField(){
    string s=commandLine->GetText();
    transcript->SetForegroundColor(1);
    transcript->AddLine((">"+s).c_str());
    commandLine->SetText("");

    int stat = cmd->exec( s );
    string reply=cmd->out.str();
    if (reply.size()>0){
        
        // try color coding, doesn't really work with TGTextView
        if(stat==0){
            transcript->SetForegroundColor(0x0000ff);
        }else{
            transcript->SetForegroundColor(0xff0000);
        }
        
        // break multiline output into lines
        std::stringstream ss( reply );
        std::string line;
        while(std::getline(ss,line,'\n')){
            transcript->AddLine( line.c_str() );
        }
    }
    if(transcript->ReturnLineCount() > 6)// visible lines(how do I know?)
        transcript->SetVsbPosition(transcript->ReturnLineCount());

    if ( (cmdHistory.size()==0) || (! (cmdHistory.back()==s))){
        cmdHistory.push_back(s);
        historyIndex=cmdHistory.size();
    }
}
void PixTestCmd::DoUpArrow(){
    if (historyIndex>0) historyIndex--;
    if (cmdHistory.size()>0){
        commandLine->SetText(cmdHistory[historyIndex].c_str());
    }
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
    
    // read the history file for future use
    ifstream inputFile(".history");
    if ( inputFile.is_open()){
        string line;
        while( getline( inputFile, line ) ){
            cmdHistory.push_back(line);  
        }
        historyIndex = cmdHistory.size();
    }

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
#include <iomanip>

#include <iostream>
#include <fstream>
#include "dictionaries.h"

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
    const char * d0 = strchr(digits, '0');

    int len = word.size();
    if ((len>0) && (word[0] == '$')) { 
        base = 16;
        i = 1;
    } else if ((len>1) && (word[0] == '0') && (word[1] == 'x')) {
        base = 16;
        i = 2;
    } else if ((len>1) && (word[0] == 'b') && ((word[1]== '0')||(word[1])=='1')) {
        base = 2;
        i = 1;
    } else {
        base = 10;
        i = 0;
    }


    int a = 0;
    const char * d;
    while ((i < len) && ((d = strchr(digits, word[i])) != NULL)) {
        // fixme verify that d-d0 <base
        a = base * a + d - d0;
        i++;
    }

    v = a;
    if (i == len) {return true; } else { return false;}

}




bool IntList::parse(Token & token,  const bool append){
    /* parse a list of integers (rocs, rows, columns..), such as
     * 1:5,8:16  or 1,2,5,7
     * hexadecimal values are allowed (see StrToI)
     * equivalent of parseIntegerList in SysCmd with wildcard support
     */
    
    if (!append) ranges.clear();

    if ( (token.front()=="*") ){
        token.pop_front();
        ranges.push_back(  make_pair( IMIN, IMAX ) );
    }else{
        int i1,i2 ; // for ranges
        do {
            if (token.front()==":"){
                i1 = IMIN;
            }else{
                if ( StrToI(token.front(), i1)) {
                    token.pop_front();
                }else{
                    return false;  // not an IntList
                }
            }
           // optionally followed by :i2 to define a range
            if ( (!token.empty()) && (token.front()==":")){
                token.pop_front();
                if (token.empty()){
                    i2=IMAX;
                }else{
                    if (StrToI(token.front(), i2)) {
                        token.pop_front();
                    }else if(token.front()=="~"){
                        token.pop_front();
                        i2=IMAX;
                    }else{
                        cout << "syntax error parsing integer list" << endl;
                        return false;
                    }
                }
            }else{
                i2=i1; // just one value
            }
            ranges.push_back( make_pair( i1, i2 ) );
            if ((ranges.size()==1) && (i1==i2)){
                singleValue = i1;
            }else{
                singleValue = UNDEFINED;
            }

            // continue until no more comma separated list elements found
            if ( (!token.empty()) && (token.front()==",")){
                token.pop_front();
            }else{
                break;
            }
        } while (!token.empty());
    // end of the list reached
  }
#ifdef DEBUG
  cout << "IntList " << ranges.size() << " single=" << (!(singleValue==UNDEFINED)) << endl;
  for(unsigned int i=0; i<ranges.size(); i++){cout << ranges[i].first << ":" << ranges[i].second << endl;}
  cout << "---------" << endl;
#endif
  return true;
}

vector<int> IntList::getVect(const int imin, const int imax){
    vector<int> IntList;
    for(unsigned int j=0; j<ranges.size(); j++){
        int i1 = ranges[j].first;
        int i2 = ranges[j].second;
        
        if( i1 == IMIN ){ i1=imin;}
        if( i2 == IMAX ){ i2=imax;}
        for(int i=i1; i<i2+1; i++){
            IntList.push_back(i);
        }
    }
    return IntList;
}
/*
vector<int> IntList::get(const vector<int> valuelist){
    vector<int> IntList;
    for(unsigned int j=0; j<ranges.size(); j++){
        int k1, k2;
        if (ranges[j].first==IMIN) {
            k1 = 0;
        }else{
            for(unsigned int k=0; k<valuelist.size(); k++){
                if (ranges[j].first==valuelist[k]) k1==k;
            }
        }
 
        if (ranges[j].second==IMAX) {
            k2 = valuelist.size();
        }else{
            for(k=0; k<valuelist.size(); k++){
                if (ranges[j].second==valuelist[k]) k2==k;
            }
        }

        for(int k=k1; k<k2; k++){
            IntList.push_back(valuelist[k]);
        }
    }
    return IntList;
}
*/

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
        }else  if ( strchr(symbols, s[i]) != NULL){
            word.push_back(s[i++]);
        }else{
            while( (i<s.size()) && (! is_whitespace(i,s)) 
                    && (strchr(symbols, s[i]) == NULL) ) {
                word.push_back(s[i++]);
            }   
        }
        words.push_back(word);
        // special treatment of whitespace following colon to allow
        // beats range notation such as pixe 5: 7:10
        if (word==":"){
            if( (i>=s.size()) || is_whitespace(i,s) ){
                words.push_back("~");
            }
        }
        word="";
    }
    return words;
}

string Target::str(){
    stringstream s;
    s<<"["<<name;
    if (name=="roc"){
        if(!expanded) { expand(0, 15);}
        for(unsigned int i=0; i<ivalues.size()-1; i++){
            s<< " " << ivalues[i] << ",";
        }
        s<<" "<<ivalues[ivalues.size()-1];
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
        lvalues.parse( token );
    }
    return true;
  }else if ((name=="tbm") || (name=="tb")) {
    token.pop_front();
    return true;
  }else{
   return false;  // not a target
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
            IntList IntList; 
            while( (!token.empty()) &&  !( (token.front()==";") || (token.front()=="]") ) ){
                if (IntList.parse(token)){
                    keyword.argv.push_back(IntList);
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

bool  Keyword::greedy_match(const char * s1, string & s2){
    if (! kw(s1) ) return false;
    s2="";
    for(unsigned int i=0; i<narg(); i++){ s2+=argv[i].str();}
    return  true;
}

bool  Keyword::match(const char * s, int & value1, int & value2){
  return  (kw(s)) && (narg()==2) && (argv[0].getInt(value1)) && (argv[1].getInt(value2));
}

bool  Keyword::greedy_match(const char * s1, int& value1, int& value2, int& value3, string & s2){
    return (kw(s1)) && (narg()>2)  && (argv[0].getInt(value1))
     && (argv[1].getInt(value2))   && (argv[2].getInt(value3))
     && concat(3, s2 );
}

bool  Keyword::match(const char * s1, string & s2){
  return  (kw(s1)) && (narg()==1) && (argv[0].getString(s2));
}


bool Keyword::match(const char * s, vector<int> & v1, vector<int> & v2){
  return  (kw(s)) && (narg()==2) && (argv[0].getVect(v1)) && (argv[1].getVect(v2));
}

bool Keyword::match(const char * s, vector<int> & v1, const int i1min, const int i1max, 
    vector<int> & v2, const int i2min, const int i2max){
  return  (kw(s)) && (narg()==2) && (argv[0].getVect(v1, i1min, i1max)) && (argv[1].getVect(v2, i2min, i2max));
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
  bool success=true;
  for (unsigned int i=0; i<stmts.size(); i++){
    success |= stmts[i]->exec(proc, target); 
  }
  return success;
}



bool Statement::exec(CmdProc * proc, Target & target){

    if (isAssignment){
            //proc->setVariable( targets.name, block );
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
            if(useTarget.name=="roc"){
                useTarget.expand( 0, 15 );
                for(unsigned int i=0; i<useTarget.size();  i++){
                    Target t = useTarget.get(i);
                    if (! (proc->process(keyword,  t))) return false;
                }
                return true;
            }else{
                return proc->process(keyword, useTarget);
            }

        }else if (!(block==NULL) ){

            // block
            if(target.name=="roc"){
                useTarget.expand( 0, 16 );
                for(unsigned int i=0; i< useTarget.size();  i++){
                    Target t = useTarget.get(i);
                    if (!(block->exec(proc, t) )) return false;
                }
                return true;
            }else{
                return block->exec(proc, useTarget); 
            }

        }else{
          
            // should not be here
            return false;

        }

    }
    return false;// should not get here
}



/* Command Processor */
CmdProc::CmdProc()
{

    verbose=false;
    defaultTarget = Target("roc",0);

}


CmdProc::CmdProc(CmdProc * p)
{

    verbose = p->verbose;
    defaultTarget = p->defaultTarget;
    macros = p->macros;
 
}

CmdProc::~CmdProc(){
}



/**************** call-backs for script processing ***********************/

int CmdProc::tb(Keyword kw){
    /* implementation of testboard commands 
     * return -1 for unrecognized commands
     * return  0 for success
     * return >0 for errors
     */
    int step, pattern, delay;
    string s, comment;
    if     ( kw.match("ia")    ){  out <<  "ia=" << fApi->getTBia(); return 0;}
    else if( kw.match("id")    ){  out <<  "id=" << fApi->getTBid(); return 0;}
    else if( kw.match("getia") ){  out <<  "ia=" << fApi->getTBia() <<"mA"; return 0;}
    else if( kw.match("getid") ){  out <<  "id=" << fApi->getTBid() <<"mA"; return 0;}
    else if( kw.match("hvon")  ){ fApi->HVon(); return 0; }
    else if( kw.match("hvoff") ){ fApi->HVoff(); return 0; }
    else if( kw.match("pon")   ){ fApi->Pon(); return 0; }
    else if( kw.match("poff")  ){ fApi->Poff(); return 0; }
    else if( kw.match("D1", s) ){ fApi->SignalProbe("D1",s);}
    else if( kw.match("D2", s) ){ fApi->SignalProbe("D2",s);}
    else if( kw.match("adc") || kw.match("dread") ){
        fApi->daqStart();
        fApi->daqTrigger(1);
        std::vector<pxar::Event> buf = fApi->daqGetEventBuffer();
        fApi->daqStop();
        for(unsigned int i=0; i<buf.size(); i++){
            out  << buf[i];
        }
         return 0;
    }
    else if( kw.match("adcraw")   ){
        fApi->daqStart();
        fApi->daqTrigger(1);
        std::vector<uint16_t> buf = fApi->daqGetBuffer();
        fApi->daqStop();
        for(unsigned int i=0; i<buf.size(); i++){
            out << " " <<hex << setw(4)<< setfill('0')  << buf[i];
        }
        return 0;
    }
    
    if( kw.greedy_match("pgset",step, pattern, delay, comment)){
        out << "pgset " << step << " " << pattern << " " << delay;
        return 0;
    }
    return -1;
}


int CmdProc::roc( Keyword kw, int rocId){
    /* implementation of roc commands 
     * return 0 if the command has not been handled here
     * otherwise execute and return some non-zero status
     *  */
    vector<int> col, row;
    int value;
    if (kw.match("vana", value))  { fApi->setDAC("Vana",value, rocId ); }
    else if (kw.match("hirange")) {fApi->setDAC("ctrlreg", fApi->_dut->getDAC(rocId, "ctrlreg")|4, rocId);}
    else if (kw.match("lorange")) {fApi->setDAC("ctrlreg", fApi->_dut->getDAC(rocId, "ctrlreg")&0xfb, rocId);}
    else if (kw.match("enable") ) {fApi->setDAC("ctrlreg", fApi->_dut->getDAC(rocId, "ctrlreg")|2, rocId);}
    else if (kw.match("disable")) {fApi->setDAC("ctrlreg", fApi->_dut->getDAC(rocId, "ctrlreg")&0xfd, rocId);}
    else if (kw.match("mask")   ) { fApi->_dut->maskAllPixels(true, rocId); }
    else if (kw.match("cald")   ) { fApi->_dut->testAllPixels(false, rocId); }
    else if (kw.match("arm", col, 0, 51, row, 0, 79) 
      || kw.match("pixd", col, 0, 51, row, 0, 79)
      || kw.match("pixe", col, 0, 51, row, 0, 79)
    ){
        //cout << col.size() << " " << row.size() << endl;
        // are "enable" and "arm" the same in apispeak?
        for(unsigned int c=0; c<col.size(); c++){
            for(unsigned int r=0; r<row.size(); r++){
                //cout << c << " " << col[c] << "  :  " << row[r] << endl;
                if (kw.keyword=="arm"){
                    fApi->_dut->testPixel(col[c], row[r], true, rocId); 
                    fApi->_dut->maskPixel(col[c], row[r], false, rocId);
                }else if (kw.keyword=="pixd"){
                    fApi->_dut->testPixel(col[c], row[r], false, rocId);
                    fApi->_dut->maskPixel(col[c], row[r], true, rocId);
                }else if (kw.keyword=="pixe"){
                    fApi->_dut->testPixel(col[c], row[r], true, rocId);
                    fApi->_dut->maskPixel(col[c], row[r], false, rocId);
                }
             }
        }

    }else{
        out << "unknown roc command " << kw.str();
        return 1;
    }
    
    
  return 0;
}


int CmdProc::tbmset(int address, int  value){
    /* emulate direct access by undoing what the api does */
   uint8_t idx = address & 0xF >> 1;  // throw away base/core for now
   const char* apinames[] = {"base0", "base2", "base4","base8","basea","basec","basee"};
   fApi->setTbmReg( apinames[ idx], value );
   return 1;
}
int CmdProc::tbmsetbit(int address, int  value){
    /* emulate direct access by undoing what the api does */
   uint8_t idx = address & 0xF >> 1;  // throw away base/core for now
   const char* apinames[] = {"base0", "base2", "base4","base8","basea","basec","basee"};
   fApi->getTbmReg( 
   fApi->setTbmReg( apinames[ idx], value );
   return 1;
}


int CmdProc::tbm(Keyword kw){
    //RegisterDictionary * _dict = RegisterDictionary::getInstance();
    int address, value;
    if (kw.match("tbmset", address, value))  { return tbmset(address, value);  }
    if (kw.match("disable","triggers")){ return tbmsetbit(address, 6, 1);}
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
    
    
    string filename;
    
    if (keyword.match("exec", filename)){
        
        // execute a file with a new command processor
        CmdProc * p = new CmdProc( this );
        ifstream inputFile( filename.c_str());
        if ( inputFile.is_open()) {
            string line;
            while( getline( inputFile, line ) ){
                p->exec(line);  
                out << p->out.str() << endl;
            }
            return true;
            
        }  else {
            
            out << " Unable to open file ";;
            return false;
        }
    }
    
    string message;
    if ( keyword.greedy_match("echo", message) || keyword.greedy_match("log",message) ){
        out << message;
        return true;
    }   



    // 
    int stat = tb( keyword );
    if ( stat >=0 ) return (stat==0);
    
    stat = tbm( keyword );
    if ( stat >=0 ) return (stat==0);
         
 
    stat = roc(keyword, target.value());

    return (stat==0);
} 





/* driver */
int CmdProc::exec(std::string s){
    
    out.str("");

    //  skip empty lines and comments
    if( (s.size()==0) || (s[0]=='#') || (s[0]=='-') ) return 0;
    
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





