// -- author: Wolfram Erdmann
// opens up a command line window

#include "PixTestCmd.hh"
#include "log.h"
#include "constants.h"

#if (defined WIN32)
#include <Windows4Root.h>  //needed before any ROOT header
#endif

#include <TGLayout.h>
#include <TGClient.h>
#include <TGFrame.h>

#include <iostream>
#include <fstream>
#include <algorithm>

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
    }
    historyIndex=cmdHistory.size();

}
void PixTestCmd::DoUpArrow(){
    if (historyIndex>0) historyIndex--;
    if (cmdHistory.size()>historyIndex){
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
    cmd = new CmdProc(  );
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
    const char * digits = "0123456789abcdef";
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
    while ((i < len) && ((d = strchr(digits, word[i])) != NULL) && (d-d0<base) ) {
        a = base * a + d - d0;
        i++;
    }

    v = a;
    if (i == len) {return true; } else { return false;}

}


int Arg::varvalue =0;


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
    }else if( (token.front()=="%") ){
        token.pop_front();
        ranges.push_back( make_pair( IVAR, IVAR ) );
        singleValue = IVAR;
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
    
    while( is_whitespace(i,s) ){i++;};
 
    string word="";
    while( i<s.size() ){


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
        while( is_whitespace(i,s) ){i++;};
 
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
  if ((name=="roc")||(name=="do")) {
    token.pop_front();
    if (!token.empty()){
        lvalues.parse( token );
    }
    return true;
  }else if ((name=="tbm") || (name=="tbma") || (name=="tbmb") || (name=="tb")) {
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
            
            // parse block
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
        block    :=  "[" <statement> ( ";" <statement> )* "]"
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
    //cout << "parsed block with size " << stmts.size() << endl;
    return true; 
}


/*********************** keyword decoding helpers**********************/


bool  Keyword::match(const char * s, int & value){
  return  (kw(s)) && (narg()==1) && (argv[0].getInt(value));
}

bool  Keyword::match(const char * s, const char * s1){
    if (narg() !=1 ) return false;
    return  (kw(s)) && (narg()==1) && (argv[0].scmp(s1));
}


bool  Keyword::match(const char * s, string & s1, vector<string> & options, stringstream & err){
    if (narg() !=1 ) return false;
    s1 = "";
    if ( (kw(s)) && (narg()==1) ){
        for(unsigned int i=0; i<options.size(); i++){
            if ( argv[0].svalue==options[i] ){
                s1 = argv[0].svalue;
                return true;
            }
        }
        err << "possible options for keyword "<< s << " are :\n";
        for(unsigned int i=0; i<options.size(); i++){
            err << " " <<options[i];
        }
        err << "\n";
        return false;
    }
    return false;
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
                    if (! (proc->process(keyword,  t, has_localTarget))) return false;
                }
                return true;
            }else if(useTarget.name=="do"){
                useTarget.expand(0, 100);
                for(unsigned int i=0; i< useTarget.size();  i++){
                    Target t = useTarget.get(i);
                    if (!(proc->process(keyword, t, false) )) return false;
                }
                return true;
                //  return proc->process(keyword, useTarget, false);
            }else{
                return proc->process(keyword, useTarget, has_localTarget);
            }

        }else if (!(block==NULL) ){

            // block
            if(target.name=="roc"){
                useTarget.expand( 0, 15 );
                for(unsigned int i=0; i< useTarget.size();  i++){
                    Target t = useTarget.get(i);
                    if (!(block->exec(proc, t) )) return false;
                }
                return true;
            }else if(useTarget.name=="do"){
                useTarget.expand(0, 100);
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
const unsigned int CmdProc::fnDAC_names=19;
const char * const CmdProc::fDAC_names[CmdProc::fnDAC_names] = 
 {"vdig","vana","vsh","vcomp","vwllpr","vwllsh","vhlddel","vtrim","vthrcomp",
 "vibias_bus","phoffset","vcomp_adc","phscale","vicolor","vcal",
 "caldel","ctrlreg","wbc","readback"};
   
CmdProc::CmdProc()
{
    /* note: fApi is not defined yet !*/
    verbose=false;
    defaultTarget = Target("roc",0);
    _dict = RegisterDictionary::getInstance();
    _probeDict = ProbeDictionary::getInstance();
    fA_names = _probeDict->getAllAnalogNames();
    fD_names = _probeDict->getAllDigitalNames();
    fPixelConfigNeeded = false;
    fTCT = 105;
    fTRC = 10;
    fTTK = 30;
    fBufsize = 10000;
    fSeq = 0;  // remember the last sequence used
 
}


CmdProc::CmdProc(CmdProc * p)
{

    verbose = p->verbose;
    defaultTarget = p->defaultTarget;
    macros = p->macros;
 
}

CmdProc::~CmdProc(){
}

/**************** implement some hardware functionalities *************/


int CmdProc::tbmset(int address, int  value){
    
    /* emulate direct access via register address */
    uint8_t core;
    uint8_t base = (address & 0xF0);
    if( base == 0xF0){ core = 1;}
    else if(base==0xE0){ core = 0;}
    else {out << "bad tbm register address "<< hex << address << dec << "\n"; return 1;};
   
    uint8_t idx = (address & 0x0F) >> 1;  
    const char* apinames[] = {"base0", "base2", "base4","base8","basea","basec","basee"};
    fApi->setTbmReg( apinames[ idx], value, core );

    return 0; // nonzero values for errors
}

int CmdProc::tbmset(string name, uint8_t coreMask, int value, uint8_t valueMask){
    /* set a tbm register, allow setting a subset of bits (thoese where mask=1)
     * the default value of mask is 0xff, i.e. all bits are changed
     * the coreMask is 1 for tbma, 2 for tbmb, 3 for both
    */
    for(size_t core=0; core<2; core++){
        if ( ((coreMask >> core) & 1) == 1 ){
            std::vector< std::pair<std::string,uint8_t> > regs = fApi->_dut->getTbmDACs(core);
            for(unsigned int i=0; i<regs.size(); i++){
                if (name==regs[i].first){
                    // found it , do something
                    uint8_t present = regs[i].second;
                    uint8_t update = value & valueMask;
                    update |= (present & (~valueMask) );
                    out << "changing tbm reg " <<  name << "["<<core<<"]";
                    out << " from 0x" << hex << (int) regs[i].second;
                    out << " to 0x" << hex << (int) update << "\n";
                    fApi->setTbmReg( name, update, core );
                }
            }
        }
     }
    return 0; // nonzero values for errors
}


int CmdProc::tbmsetbit(string name, uint8_t coreMask, int bit, int value){
    /* set individual bits */
    return tbmset(name, coreMask, (value & 1)<<bit, 1<<bit);
}



int CmdProc::adctest(const string signal){
    unsigned int n=1024;
    vector<double> y;
    if ( fApi->daqADC(signal, n, y) ){
        //if (y.size()<n){ out<<"incomplete, only "<<y.size()<< " words read\n";}
        //cout << "y.size=" << y.size() << endl;
        //for(unsigned int i=0; i<n; i++){ cout << i << "  " << y[i] << endl;}
        sort( y.begin(), y.end());
        unsigned int m=y.size();
        double low(0), high(0), ny(0);
        for(unsigned int i=int(0.1*m); i<int(0.3*m); i++){
            low += y[i];
            high += y[m-i];
            ny++;
        }
        out << signal << "  low level=" << low/ny << " high level = " << high/ny << "\n";
    }else{
        out << "error getting adc data";
    }
    return 0;
}



int CmdProc::sequence(int seq){
    vector< pair<string, uint8_t> > pgsetup;
    pgsetup.push_back( make_pair( "sync", 10) );
    if (seq & 0x08 ) { pgsetup.push_back( make_pair( "resr", fTRC) ); }
    if (seq & 0x04 ) { pgsetup.push_back( make_pair("cal", fTCT )); }
    if (seq & 0x02 ) { pgsetup.push_back( make_pair("trg", fTTK )); }
    if (seq & 0x01 ) { pgsetup.push_back( make_pair("token", 1)); }
    for(unsigned int i=0; i<(fBufsize-fTTK-20)/256; i++){
        pgsetup.push_back(make_pair("none", 255));
    }
    pgsetup.push_back(make_pair("none", 255));
    pgsetup.push_back(make_pair("none", 0));
    fApi->setPatternGenerator(pgsetup);
    fSeq = seq;
    return 0;
}


/**************** call-backs for script processing ***********************/

int CmdProc::tb(Keyword kw){
    /* handle testboard commands 
     * return -1 for unrecognized commands
     * return  0 for success
     * return >0 for errors
     */
    int step, pattern, delay,value;
    string s, comment;
    if( kw.match("ia")    ){  out <<  "ia=" << fApi->getTBia(); return 0;}
    if( kw.match("id")    ){  out <<  "id=" << fApi->getTBid(); return 0;}
    if( kw.match("getia") ){  out <<  "ia=" << fApi->getTBia() <<"mA"; return 0;}
    if( kw.match("getid") ){  out <<  "id=" << fApi->getTBid() <<"mA"; return 0;}
    if( kw.match("hvon")  ){ fApi->HVon(); return 0; }
    if( kw.match("hvoff") ){ fApi->HVoff(); return 0; }
    if( kw.match("pon")   ){ fApi->Pon(); return 0; }
    if( kw.match("poff")  ){ fApi->Poff(); return 0; }
    if( kw.match("d1", s, fD_names, out ) ){ fApi->SignalProbe("D1",s); return 0;}
    if( kw.match("d2", s, fD_names, out ) ){ fApi->SignalProbe("D2",s); return 0;}
    if( kw.match("a1", s, fA_names, out ) ){ fApi->SignalProbe("A1",s); return 0;}
    if( kw.match("a2", s, fA_names, out ) ){ fApi->SignalProbe("A2",s); return 0;}
    if( kw.match("adc", s, fA_names, out ) ){ fApi->SignalProbe("adc",s); return 0;}  
    if( kw.match("seq", pattern)){ sequence(pattern); return 0;}
    if( kw.match("seq","t")){ sequence( 2 ); return 0; }
    if( kw.match("seq","ct")){ sequence( 6 ); return 0; }
    if( kw.match("seq","rct")){ sequence( 14 ); return 0; }
    if( kw.match("seq","rctt")){ sequence( 15 ); return 0; }
    //if( kw.match("res"){ int s0=fSeq; sequence( 8 ); pg_single(); sequence(s0);}
    if( kw.match("adctest", s, fA_names, out ) ){ adctest(s); return 0;}  
    if( kw.match("tct", value)){ fTCT=value; if (fSeq>0){sequence(fSeq);} return 0;}
    if( kw.match("ttk", value)){ fTTK=value; if (fSeq>0){sequence(fSeq);} return 0;}
    if( kw.match("trc", value)){ fTRC=value; if (fSeq>0){sequence(fSeq);} return 0;}
    if( kw.match("adc") || kw.match("dread") ){
        fApi->daqStart(fBufsize, fPixelConfigNeeded);
        fApi->daqTrigger(1, fBufsize);
        std::vector<pxar::Event> buf = fApi->daqGetEventBuffer();
        fApi->daqStop(false);
        for(unsigned int i=0; i<buf.size(); i++){
            out  << buf[i];
        }
        out << " ["<<buf.size()<<"]\n";
        fPixelConfigNeeded = false;
        return 0;
    }
    if( kw.match("raw")   ){
        fApi->daqStart(fBufsize, fPixelConfigNeeded);
        fApi->daqTrigger(1, fBufsize);
        std::vector<uint16_t> buf = fApi->daqGetBuffer();
        fApi->daqStop(false);
        out << dec << buf.size() << " words read:";
        for(unsigned int i=0; i<buf.size(); i++){
            out << " " <<hex << setw(4)<< setfill('0')  << buf[i];
        }
        out << "["<< dec << buf.size() <<"]\n";
        fPixelConfigNeeded = false;
        return 0;
    }
    
    if( kw.greedy_match("pgset",step, pattern, delay, comment)){
        out << "pgset " << step << " " << pattern << " " << delay;
        return 0;
    }
    return -1;
}


int CmdProc::roc( Keyword kw, int rocId){
    /* handle roc commands 
     * return -1 for unrecognized commands
     * return  0 for success
     * return >0 for errors
     */
    vector<int> col, row;
    int value;
    for(unsigned int i=0; i<fnDAC_names; i++){
        if (kw.match(fDAC_names[i],value)){ fApi->setDAC(kw.keyword,value, rocId );  return 0 ;  }
    }
    if (kw.match("hirange")) {fApi->setDAC("ctrlreg", (fApi->_dut->getDAC(rocId, "ctrlreg"))|4, rocId);return 0 ;}
    if (kw.match("lorange")) {fApi->setDAC("ctrlreg", (fApi->_dut->getDAC(rocId, "ctrlreg"))&0xfb, rocId);return 0 ;}
    if (kw.match("disable") ) {fApi->setDAC("ctrlreg", (fApi->_dut->getDAC(rocId, "ctrlreg"))|2, rocId);return 0 ;}
    if (kw.match("enable")) {fApi->setDAC("ctrlreg", (fApi->_dut->getDAC(rocId, "ctrlreg"))&0xfd, rocId);return 0 ;}
    if (kw.match("mask")   ) { fApi->_dut->maskAllPixels(true, rocId); fPixelConfigNeeded = true; return 0 ;}
    if (kw.match("cald")   ) { fApi->_dut->testAllPixels(false, rocId); fPixelConfigNeeded = true; return 0 ;}
    if (kw.match("arm", col, 0, 51, row, 0, 79) 
      || kw.match("pixd", col, 0, 51, row, 0, 79)
      || kw.match("pixe", col, 0, 51, row, 0, 79)
    ){
        //cout << col.size() << " " << row.size() << endl;
        // are "enable" and "arm" the same in apispeak?
        for(unsigned int c=0; c<col.size(); c++){
            for(unsigned int r=0; r<row.size(); r++){
                if(verbose) { cout << kw.keyword << " roc " << rocId << "  " << col[c] << "  :  " << row[r] << endl; }
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
        fPixelConfigNeeded = true;

    return 0 ;
  }
    
  return -1;
}


int CmdProc::tbm(Keyword kw, int cores){
    /* handle tbm commands 
     * core=1 = TBMA, 2=TBMB, 3=both
     * return -1 for unrecognized commands
     * return  0 for success
     * return >0 for errors
     */
     if(verbose) { cout << "tbm " << kw.keyword << "cores =" << cores << endl;}
    int address, value;
    if (kw.match("tbmset", address, value))  { return tbmset(address, value);  }
    if (kw.match("enable", "pkam")     ){ return tbmsetbit("base0",cores, 0, 0);}
    if (kw.match("disable","pkam")     ){ return tbmsetbit("base0",cores, 0, 1);}
    if (kw.match("accept", "triggers") ){ return tbmsetbit("base0",cores, 4, 0);}
    if (kw.match("ignore", "triggers") ){ return tbmsetbit("base0",cores, 4, 1);}
    if (kw.match("enable", "triggers") ){ return tbmsetbit("base0",cores, 6, 0);}
    if (kw.match("disable","triggers") ){ return tbmsetbit("base0",cores, 6, 1);}
    if (kw.match("enable", "autoreset")){ return tbmsetbit("base0",cores, 7, 0);}
    if (kw.match("disable","autoreset")){ return tbmsetbit("base0",cores, 7, 1);}
    if (kw.match("mode","cal"  )    ){ return tbmset("base2",cores,   0xC0);}
    if (kw.match("mode","clear")    ){ return tbmset("base2",cores,   0x00);}
    if (kw.match("inject","trg")    ){ return tbmset("base4",cores,      1);}
    if (kw.match("reset","roc")     ){ return tbmset("base4",cores, (1<<2));}
    if (kw.match("inject","cal")    ){ return tbmset("base4",cores, (1<<3));}
    if (kw.match("reset","tbm")     ){ return tbmset("base4",cores, (1<<4));}
    if (kw.match("clear","stack")   ){ return tbmset("base4",cores, (1<<5));}
    if (kw.match("clear","token")   ){ return tbmset("base4",cores, (1<<6));}
    if (kw.match("clear","counter") ){ return tbmset("base4",cores, (1<<7));}
    if (kw.match("pkamcounter",value)){return tbmset("base8",cores,  value);}
    if (kw.match("autoreset",value)  ){return tbmset("basec",cores,  value);}
    if (kw.match("dly0",value)  ){return tbmset("basea", cores,  (value&0x7)   , 0x07);}
    if (kw.match("dly1",value)  ){return tbmset("basea", cores,  (value&0x7)<<3, 0x38);}
    if (kw.match("dlyhdr",value)){return tbmset("basea", cores,  (value&0x1)<<6, 0x40);}
    if (kw.match("dlytok",value)){return tbmset("basea", cores,  (value&0x1)<<7, 0x80);}
    if (kw.match("phase400",value) ){return tbmset("basee", 1, (value&0x7)<<2, 0x1c);}
    if (kw.match("phase160",value) ){return tbmset("basee", 1, (value&0x7)<<5, 0x70);}
    return -1; // nothing done
}


bool CmdProc::process(Keyword keyword, Target target, bool forceTarget){
    /* process a parsed command line command
     * called by the exec method of Statements
     * if the forceTarget flag is True, the keyword is only processed by
     * the corresponding target, otherwise all of them are tried
     *  */

    // debugging printout
    if (verbose){
        cout << "keyword  -->  " << keyword.str() << " " << target.str() << endl;   
    }

    if (target.name=="do"){
        Arg::varvalue = target.value();
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
    
    if (keyword.match("verbose")){ verbose=true; return true;}
    if (keyword.match("quiet")){ verbose=false; return true;}
    if (keyword.match("target")){ out << defaultTarget.str(); return true;}
   
    string message;
    if ( keyword.match("echo","roc")){ out << "roc " << target.value() << "\n"; return true;}
    if ( keyword.match("echo","%")){ out << "%" << target.value() << "\n"; return true;}
    if ( keyword.greedy_match("echo", message) || keyword.greedy_match("log",message) ){
        out << message << "\n";
        return true;
    }   



    int stat  =0;

    // target explicitely specified
    if ( forceTarget ){
        if (target.name=="tb") {
            stat =  tb( keyword );
            if ( stat >=0 ) return (stat==0);
        }
        
        else if (target.name=="tbm")  {
            stat =  tbm( keyword );
            if ( stat >=0 ) return (stat==0);
        }
        
        else if (target.name=="tbma")  {
            stat =  tbm( keyword, 1 );
            if ( stat >=0 ) return (stat==0);
        }
        
        else if (target.name=="tbmb")  {
            stat =  tbm( keyword, 2 );
            if ( stat >=0 ) return (stat==0);
        }
        
        else if (target.name=="roc")  {
            stat =  roc(keyword, target.value());
            if ( stat >=0 ) return (stat==0);
        }
        out << "unknown " << target.name << " command";
        return false;
    }

    // if not, resolve target by keyword
    stat = tb( keyword );
    if ( stat >=0 ) return (stat==0);
    
    stat = tbm( keyword ); // hmmm, how does that work for default targets?
    if ( stat >=0 ) return (stat==0);
         
 
    stat = roc(keyword, target.value());
    if ( stat >=0 ) return (stat==0);

    out << "unrecognized command";
    return false;
} 





/* driver */
int CmdProc::exec(std::string s){
    
    out.str("");

    //  skip empty lines and comments
    if( (s.size()==0) || (s[0]=='#') || (s[0]=='-') ) return 0;


    // pre-processing: to lower
    std::string t="";
    for(unsigned int i=0; i<s.size(); i++){
        //if ( !(s[i]=='_') ){ fixme: removing all underscores leads to problems with dacs, must do it for binary numbers only
            t.push_back( tolower( s[i] ));
        //}
    }
    s=t;
    
    // parse and execute a string, leads to call-backs to CmdProc::process
    Token words( getWords(s) );
    words.macros=&macros;
    macros["test"] = getWords("roc 0 vana 5");

    int j=0;
    //parse
    vector < Statement *> stmts;
    while( (!words.empty()) && (j++ < 2000)){
        Statement * c = new Statement; 
        bool stat=  c->parse( words );
        if( stat ){
            stmts.push_back( c );
        }
        if (!words.empty()) {
            if (words.front()==";") {
                words.pop_front();
            }else{
                out << "expected ';' instead of '" << words.front() << "'";
                return 1;
            }
        }
    }

    bool ok = true;
    for( unsigned int i=0; i<stmts.size(); i++){
        ok |= stmts[i]->exec(this, defaultTarget);
    }

    if (ok){
        return 0;
    }else{
        return 2;
    }
}





