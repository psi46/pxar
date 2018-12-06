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
#include <bitset>

#include "timer.h"
#include "helper.h"

// #define DEBUG

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
  fout.close();
}

void PixTestCmd::DoTextField(){
    string s=commandLine->GetText();
    transcript->SetForegroundColor(1);
    transcript->AddLine((">"+s).c_str());
    commandLine->SetText("");
    transcript->ShowBottom();
    
    ULong_t fBusyColor, fReadyColor;
    gClient->GetColorByName("DarkGray", fBusyColor);
    gClient->GetColorByName("White", fReadyColor);
  
    commandLine->SetBackgroundColor(fBusyColor);
    gSystem->ProcessEvents();
    
    
    cmd->exec( s );
    string reply=cmd->out.str();
    if (reply.size()>0){
        
        // break multiline output into lines
        std::stringstream ss( reply );
        std::string line;
        int linecount=0;
        while(std::getline(ss,line,'\n')){
            transcript->AddLine( line.c_str() );
            linecount++;
            if (((linecount%100000)==0)&&(linecount>0)){
                cout << "are you sure, this is line " << linecount << "\n" << line.c_str() << endl;
            }
        }
    }

    transcript->ShowBottom();
    commandLine->SetBackgroundColor(fReadyColor);

    if ( (cmdHistory.size()==0) || (! (cmdHistory.back()==s))){
        cmdHistory.push_back(s);
    }
    historyIndex=cmdHistory.size();

}

void PixTestCmd::flush(string s){
    if (s.size()>0){
        
        // break multiline output into lines
        std::stringstream ss( s );
        std::string line;
        int linecount=0;
        while(std::getline(ss,line,'\n')){
            transcript->AddLine( line.c_str() );
            linecount++;
            if (((linecount%100000)==0)&&(linecount>0)){
                cout << "are you sure, this is line " << linecount << "\n" << line.c_str() << endl;
            }
        }
    }

    transcript->ShowBottom();
    gSystem->ProcessEvents();
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
    tf = new TGTransientFrame(gClient->GetRoot(), main, 600, 800);//w,h
    
    // == Transcript ============================================================================================

    textOutputFrame = new TGHorizontalFrame(tf, w, 400);
    transcript = new TGTextView(textOutputFrame, w, 400);
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
    cmd = new CmdProc( this );
    cmd->setApi(fApi, fPixSetup);

    PixTest::update();

}

void PixTestCmd::stopTest()
{
    if (cmd){
        cmd->stop(true);
    }
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
        singleValue = UNDEFINED;
    }else if( (token.front()=="%") ){
        token.pop_front();
        ranges.push_back( make_pair( IVAR, IVAR ) );
        singleValue = IVAR;
    }else{
        int i1,i2 ; // for ranges
        do {
            if (token.front()==":"){
                cout << "this shouldn't really happen anymore" << endl;
                i1 = IMIN;
                i2 = IMAX; // in case nothing else follows
            }else if( token.front()=="~" ){
                i1=IMIN;
                token.pop_front();
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
  cout << "IntList  #ranges=" << ranges.size() << " single=" << (!(singleValue==UNDEFINED)) << endl;
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
        else if (i1 == IVAR ){ i1=Arg::varvalue;}
        
        if( i2 == IMAX ){ i2=imax;}
        else if(i2 == IVAR){ i2=Arg::varvalue;}
        
        for(int i=i1; i<i2+1; i++){
            IntList.push_back(i);
        }
    }
    return IntList;
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
        
        if(word==":"){
            // special treatment of whitespace around colons to allow
            // Beats range notation such as pixe 5: 7:10
            if((i>1)&& is_whitespace(i-2,s)){
                words.push_back("~");
            }
            words.push_back(word);
            if( (i>=s.size()) || is_whitespace(i,s) ){
                words.push_back("~");
            }
        }else{
            words.push_back(word);
        }
        
        word="";
        while( is_whitespace(i,s) ){i++;};
 
    }

#ifdef DEBUG
    cout<< "getwords" <<endl;
    for(unsigned int i=0; i< words.size(); i++){ cout << i << " " << words[i] << endl;}
#endif

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
  }else if ((name=="tbm") || (name=="tbma") || (name=="tbmb")  || (name=="tbm0a") || (name=="tbm0b") || (name=="tbm1a") || (name=="tbm1b")|| (name=="tb")) {
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
                cout << "Statement::parse adding macro " << name << endl;
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
            while( (!token.empty()) &&  !( (token.front()==";") || (token.front()=="]") || (token.front()==">")  ) ){
                if (IntList.parse(token)){
                    keyword.argv.push_back(IntList);
                }else{     
                    keyword.argv.push_back(token.front());
                    token.pop_front();
                }
            }

        }else{
            cerr << "syntax error?" << endl;
            return false;
        }
        
        // redirection ?
        if(!(token.empty()) && (token.front()==">")){
            token.pop_front();
            if (!(token.empty())){
                out_filename = token.front();
                token.pop_front();
                redirected=true;
            }else{
                cerr << "expected filename after '>' " << endl;
                return false;
            }
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

bool  Keyword::match(const char * s, int & value, const char * s1){
  return  (kw(s)) && (narg()==2) && (argv[0].getInt(value))  && (argv[1].scmp(s1)) ;
}

bool  Keyword::match(const char * s1, const char * s2, int & value){
  return  (kw(s1)) && (narg()==2) && (argv[0].scmp(s2)) && (argv[1].getInt(value));
}

bool  Keyword::match(const char * s1, const char * s2, int&value1, int&value2){
  return  (kw(s1)) && (narg()==3) && (argv[0].scmp(s2))
    && (argv[1].getInt(value1))&& (argv[2].getInt(value2));
}

bool  Keyword::match(const char * s1, const char * s2, int&value1, int&value2, int&value3){
  return  (kw(s1)) && (narg()==4) && (argv[0].scmp(s2))
    && (argv[1].getInt(value1)) && (argv[2].getInt(value2))  && (argv[3].getInt(value3));
}

bool  Keyword::match(const char * s, const char * s1){
    if (narg() !=1 ) return false;
    return  (kw(s)) && (narg()==1) && (argv[0].scmp(s1));
}

bool  Keyword::match(const char * s, const char * s1, string & s2){
    if (narg() !=1 ) return false;
    return  (kw(s)) && (narg()==2) && (argv[0].scmp(s1))  && (argv[1].getString(s2));
}

bool  Keyword::match(const char * s, string & s1, vector<string> & options, ostream & err){
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


bool  Keyword::match(const char * s, string & s1, vector<string> & options, int & value,  ostream & err){
    if (narg() !=2 ) return false;
    s1 = "";
    if ( kw(s) && argv[1].getInt(value) ){
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

bool  Keyword::match(const char * s, int & value1, int & value2, int & value3){
  return  (kw(s)) && (narg()==3) && (argv[0].getInt(value1)) && (argv[1].getInt(value2)) && (argv[2].getInt(value3));
}

bool  Keyword::match(const char * s, int & value1, int & value2, int & value3, int & value4){
  return  (kw(s)) && (narg()==4) && (argv[0].getInt(value1)) && (argv[1].getInt(value2)) 
    && (argv[2].getInt(value3)) && (argv[3].getInt(value4));
}

bool  Keyword::match(const char * s, int & value1, int & value2, int & value3, int & value4, int & value5){
  return  (kw(s)) && (narg()==5) && (argv[0].getInt(value1)) && (argv[1].getInt(value2)) 
    && (argv[2].getInt(value3)) && (argv[3].getInt(value4)) && (argv[4].getInt(value5));
}

bool  Keyword::greedy_match(const char * s1, int& value1, int& value2, int& value3, string & s2){
    return (kw(s1)) && (narg()>2)  && (argv[0].getInt(value1))
     && (argv[1].getInt(value2))   && (argv[2].getInt(value3))
     && concat(3, s2 );
}

bool  Keyword::match(const char * s1, string & s2){
  return  (kw(s1)) && (narg()==1) && (argv[0].getString(s2));
}

bool  Keyword::match(const char * s1, vector<int> & v1){
  return  (kw(s1)) && (narg()==1) &&  (argv[0].getVect(v1));
}

bool  Keyword::match(const char * s1, string & s2, vector<int> & v1){
  return  (kw(s1)) && (narg()==2) && (argv[0].getString(s2)) && (argv[1].getVect(v1));
}

bool  Keyword::match(const char * s1, string & s2, int& value1, int& value2, int& value3){
  return  (kw(s1)) && (narg()==4) && (argv[0].getString(s2))  
     && (argv[1].getInt(value1))
     && (argv[2].getInt(value2))   && (argv[3].getInt(value3));
}

bool Keyword::match(const char * s, vector<int> & v1, vector<int> & v2){
  return  (kw(s)) && (narg()==2) && (argv[0].getVect(v1)) && (argv[1].getVect(v2));
}

bool Keyword::match(const char * s, vector<int> & v1, const int i1min, const int i1max, 
    vector<int> & v2, const int i2min, const int i2max){
  return  (kw(s)) && (narg()==2) && (argv[0].getVect(v1, i1min, i1max)) && (argv[1].getVect(v2, i2min, i2max));
}

bool Keyword::match(const char * s, vector<int> & v1, const int i1min, const int i1max, 
    vector<int> & v2, const int i2min, const int i2max, int & value){
  return  (kw(s)) && (narg()==3) 
    && (argv[0].getVect(v1, i1min, i1max)) 
    && (argv[1].getVect(v2, i2min, i2max)
    && (argv[2].getInt(value)));
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
        
        
        bool stat=false;
        if ( (block==NULL) && !(keyword.keyword=="") ){
            // keyword + arguments ==> execute
            if(useTarget.name=="roc"){
                useTarget.expand( 0, 15 );
                for(unsigned int i=0; i<useTarget.size();  i++){
                    Target t = useTarget.get(i);
                    if (! (proc->process(keyword,  t, has_localTarget, i))) return false;
                }
                stat = true;
            }else if(useTarget.name=="do"){
                useTarget.expand(0, 100);
                for(unsigned int i=0; i< useTarget.size();  i++){
                    Target t = useTarget.get(i);
                    Arg::varvalue = t.value();
                    if (!(proc->process(keyword, t, false) )) return false;
                }
                stat = true;
            }else{
                stat = proc->process(keyword, useTarget, has_localTarget);
            }

        }else if (!(block==NULL) ){

            // block
            if(useTarget.name=="roc"){
                useTarget.expand( 0, 15 );
                for(unsigned int i=0; i< useTarget.size();  i++){
                    Target t = useTarget.get(i);
                    if (!(block->exec(proc, t) )) return false;
                }
                stat =  true;
            }else if(useTarget.name=="do"){
                useTarget.expand(0, 100);
                for(unsigned int i=0; i< useTarget.size();  i++){
                    Target t = useTarget.get(i);
                    Arg::varvalue = t.value();
                    if (!(block->exec(proc, t) )) return false;
                }
                stat =  true;
            }else{
                stat = block->exec(proc, useTarget); 
            }

        }else{
          
            // should not be here
            stat = false;

        }
        
        if( redirected ){
            ofstream fout;
            fout.open( out_filename.c_str());
            if(fout){
                fout << proc->out.str();
                proc->out.str("");
                proc->redirected=false;
                fout.close();
            }else{
                cerr << "unable to open " << out_filename << endl;
            }
        }
        return stat;

    }
    return false;// should not get here
}



/* Command Processor */
const unsigned int CmdProc::fnDAC_names=19;
const char * const CmdProc::fDAC_names[CmdProc::fnDAC_names] = 
 {"vdig","vana","vsh","vcomp","vwllpr","vwllsh","vhlddel","vtrim","vthrcomp",
 "vibias_bus","phoffset","vcomp_adc","phscale","vicolor","vcal",
 "caldel","ctrlreg","wbc","readback"};
 
int CmdProc::fGetBufMethod = 1; // 0:daqGetBuffer  1:daqGetRawEventBuffer
int CmdProc::fNtrigTimingTest  = 100;
int CmdProc::fPrerun=0;
bool CmdProc::fFW35=false;
bool CmdProc::fStopWhateverYouAreDoing = false;
int CmdProc::fIgnoreReadbackErrors = false;

void CmdProc::init()
{
    /* note: fApi may not be defined yet !*/
    verbose=false;
    redirected=false;
    //fIgnoreReadbackErrors=false;  now static
    fDumpFlawed=FLAG_DUMP_FLAWED_EVENTS;
    master = NULL;
    fEchoExecs = true;
    defaultTarget = Target("roc",0);
    _dict = RegisterDictionary::getInstance();
    _probeDict = ProbeDictionary::getInstance();
    fA_names = _probeDict->getAllAnalogNames();
    fD_names = _probeDict->getAllDigitalNames();


    fGetBufMethod = 1;
    fPixelConfigNeeded = true;
    fTCT = 106;
    fTRC = 10;
    fTTK = 30;
    fMaxPeriod = 10000;
    fBufsize = 100000; // DTB_SOURCE_BUFFER_SIZE makes things slow, only increase where needed
    fSeq = 7;  // pg sequence bits
    fPeriod = 0;
    fPgRunning = false;
    macros["start"] = getWords("[roc * mask; roc * cald; reset tbm; seq 14]");
    macros["startroc"] = getWords("[mask; seq 15; arm 20 20; tct 106; vcal 200; adc]");
    macros["tbmonly"] = getWords("[reset tbm; tbm disable triggers; seq 10; adc]");
    macros["unmask"] = getWords("[roc * enable; roc * pixe * *; reset roc; reset tbm]");
    out.str("");
}


void CmdProc::setApi(pxar::pxarCore * api, PixSetup * setup){
    /* set api instance and do related setups */
    fApi = api;
    fPixSetup = setup;
    
    ConfigParameters *p = fPixSetup->getConfigParameters();
    if (fSigdelaysSetup.size()==0){
        fSigdelaysSetup = p->getTbSigDelays();
        fSigdelays = p->getTbSigDelays();
    }
   
    // define readout mapping
    fDaqChannelRocIdOffset.reserve( nDaqChannelMax );  
    fnTbmCore =   fApi->_dut->getNTbms();
    fTbmChannels.clear();   // daq channels connected to a tbm (bit-pattern) [fnTbm]
    if(layer1()){
        fnTbm = 2;
        fnRocPerChannel=2;
        fnCoresPerTBM = 2;
        fnDaqChannel=8;
        fDaqChannelRocIdOffset[0]= 4;
        fDaqChannelRocIdOffset[1]= 6;
        fDaqChannelRocIdOffset[2]= 8;
        fDaqChannelRocIdOffset[3]= 10;
        fDaqChannelRocIdOffset[4]= 12;
        fDaqChannelRocIdOffset[5]= 14;
        fDaqChannelRocIdOffset[6]= 0;
        fDaqChannelRocIdOffset[7]= 2;
        fTbmChannels.push_back(0xf0);
        fTbmChannels.push_back(0x0f);

    }else if(tbm08()){
        fnTbm = 1;
        fnRocPerChannel=8;
        fnCoresPerTBM = 1;
        fnDaqChannel=2;
        fDaqChannelRocIdOffset[0]= 0; 
        fDaqChannelRocIdOffset[1]= 8;
        fTbmChannels.push_back(0x03);
    }else{ //09
        fnTbm = 1;
        fnRocPerChannel=4;
        fnCoresPerTBM = 2;
        fnDaqChannel=4;
        fDaqChannelRocIdOffset[0]= 0; 
        fDaqChannelRocIdOffset[1]= 4;
        fDaqChannelRocIdOffset[2]= 8; 
        fDaqChannelRocIdOffset[3]=12;
        fTbmChannels.push_back(0x0f);
}
 
}


void CmdProc::flush(stringstream & o){
    if ( redirected || (master==NULL) ) return;
    master->flush(o.str());
    o.str("");
}



CmdProc::CmdProc(CmdProc * p)
{
    init();
    verbose = p->verbose;
    redirected = p->redirected;
    fDumpFlawed= p->fDumpFlawed;
    // fIgnoreReadbackErrors = p->fIgnoreReadbackErrors; now static
    master = p->master;
    fEchoExecs = p->fEchoExecs;
    defaultTarget = p->defaultTarget;
    fSeq = p->fSeq;
    fPeriod = p->fPeriod;
    fPgRunning = p->fPgRunning;
    fTCT = p->fTCT;
    fTRC = p->fTRC;
    fTTK = p->fTTK;
    fBufsize=p->fBufsize;
    macros = p->macros;
    fSigdelaysSetup = p->fSigdelaysSetup;
    fSigdelays = p->fSigdelays; // what'ts the difference ?
    setApi( p->fApi, p->fPixSetup);
}

CmdProc::~CmdProc(){
}

/**************** implement some hardware functionalities *************/


int CmdProc::tbmset(int address, int  value){
    
    /* emulate direct access via register address 
     * higher bits select the tbm 0 or 1 in case of layer 1 modules */
    uint8_t core=0;
    uint8_t base = (address & 0xF0);
    uint8_t tbm = (address & 0x100);
    if( base == 0xF0){ core = 1;}
    else if(base==0xE0){ core = 0;}
    else {out << "bad tbm register address "<< hex << address << dec << "\n"; return 1;};
   
	if (tbm==1){
		core +=2;
	}
	
    uint8_t idx = (address & 0x0F) >> 1;  
    const char* apinames[] = {"base0", "base2", "base4","invalid","base8","basea","basec","basee"};
    fApi->setTbmReg( apinames[ idx], value, core );
    return 0; // nonzero values for errors
}



int CmdProc::tbmget(string name, const uint8_t coreMask, uint8_t & value){
    /* get a tbm register value as cached by the api
     * when multiple cores are present int the core mask, only the first value is returned
    */
    int error=1;
    for(uint8_t core=0; core<fApi->_dut->getNEnabledTbms(); core++){
		if (( (coreMask >> core) & 1 ) == 1){
			std::vector< std::pair<std::string,uint8_t> > regs = fApi->_dut->getTbmDACs(core);
			for(unsigned int i=0; i<regs.size(); i++){
				if (name==regs[i].first){ value = regs[i].second; error=0;}
			}
			return error;
		}
	}

    return error; // nonzero values for errors
}


int CmdProc::tbmset(string name, uint8_t coreMask, int value, uint8_t valueMask){
    /* set a tbm register, allow setting a subset of bits (thoese where mask=1)
     * the default value of mask is 0xff, i.e. all bits are changed
     * the cores = 0:TBMA(0), 1:TBMB(0), 2:TBMA(1), 4:TBMB(1)
     * combinations are possible
     * 
    */
    if ((value & (~valueMask) )>0) {
        out << "Warning! tbm set value " << hex << (int) value 
            << " has bits outside mask ("<<hex<< (int) valueMask << ")\n";
    }
         
    
    int err=1;
    //out << "tbmset: " << name << " " <<  (int) coreMask << " " << value << "  " << bitset<8>(valueMask) << "\n";
    for(size_t core=0; core < fApi->_dut->getNEnabledTbms(); core++){
        if ( ((coreMask >> core) & 1) == 1 ){
            std::vector< std::pair<std::string,uint8_t> > regs = fApi->_dut->getTbmDACs(core);
            if (regs.size()==0) {
                out << "TBM registers not set !?! This is not going to work.\n";
            }
            for(unsigned int i=0; i<regs.size(); i++){
                if (name==regs[i].first){
                    // found it , do something
                    uint8_t present = regs[i].second;
                    uint8_t update = value & valueMask;
                    update |= (present & (~valueMask) );
                    if(verbose){
                        out << "changing tbm reg " <<  name << "["<<core<<"]";
                        out << " from 0x" << hex << (int) regs[i].second;
                        out << " to 0x" << hex << (int) update << "\n";
                    }
                    fApi->setTbmReg( name, update, core );
                    err=0;
                }
            }
        }
     }
    return err; // nonzero values for errors
}


int CmdProc::tbmsetbit(string name, uint8_t coreMask, int bit, int value){
    /* set individual bits */
    return tbmset(name, coreMask, (value & 1)<<bit, 1<<bit);
}



int CmdProc::countHits(){
    int nhit=0;
    vector<DRecord > data;
    int stat = runDaqRaw(fBuf, 1, 0,  0);
    if (stat>0) return -1;
    stat = getData(fBuf, data, 0);
    if (stat>0) return -2;
    for(unsigned int i=0; i<data.size(); i++){
        if (data[i].type==0) nhit++;
    }
    return nhit;
}


vector<int> CmdProc::countHits( vector<DRecord> data, size_t nroc){
    vector<int> nhit(nroc);
    for(unsigned int i=0; i<data.size(); i++){
        if ( (data[i].type==DRecord::HIT) && (data[i].id<nroc) ){
            nhit[ data[i].id ] ++;
        }
    }
    return nhit;
}


int CmdProc::countErrors(unsigned int ntrig, int ftrigkhz, int nroc, bool setup){
    int stat = runDaqRaw(fBuf, ntrig, ftrigkhz, 0, setup);
    if (stat>0) return -1; // no data
    vector<DRecord > data;
    data.clear();
    int verbosity = verbose ? 1 : 0;
    stat = getData(fBuf, data, verbosity, nroc, true);
    if (stat>0){
        return stat;
    }else{
        if  (fNumberOfEvents==ntrig) return 0;
        if(verbose) cout << "number of events (" << fNumberOfEvents << ") does not match triggers "<< ntrig << endl;
        return 999;
    }
}


int CmdProc::countGood(unsigned int nloop, unsigned int ntrig, int ftrigkhz, int nroc){
    /* run loops eve events with ntrig triggers each ,
     * returns the total number good loops
     * the result separated by channel is available in fGoodLoops */
     
    tbmset("base4", ALLTBMS, 0x80);// reset tbm event counter, both cores
    int good=0;
    clear_DaqChannelCounter( fGoodLoops );
        
    setupDaq(ntrig, ftrigkhz, 0);
    for(unsigned int k=0; k<nloop; k++){
        if (fFW35) tbmset("base4", ALLTBMS, 0x80); // temp fix, avoid event counter reaching 255
        if(fPrerun>0) runDaqRaw(fPrerun, 10);
        int nerr = countErrors(ntrig, ftrigkhz, nroc, false);
        if(nerr==0){
            good++;
        }

        for( unsigned int i=0;i<8; i++){  //legacy code
            fDeser400XOR1sum[i] += ( ((fDeser400XOR[0]|fDeser400XOR[1]) >> i) & 1);
         }

         // channel-wise book-keeping
         for( unsigned int i=0; i<nDaqChannelMax; i++ ) {
             if ((fDaqErrorCount[i] == 0)  && (fNEvent[i]==ntrig) ){
                 fGoodLoops[i]++;
             }
         }
    }
    restoreDaq();
    return good;
}


int CmdProc::tctscan(unsigned int tctmin, unsigned int tctmax){
    unsigned int tct0 = fTCT;
    
    if (tctmin==tctmax){
        tctmin = (fTCT>20) ? fTCT - 20 : 0;
        tctmax = fTCT + 20;
    }
    
    std::vector< uint8_t > rocids = fApi->_dut->getRocI2Caddr();
    if (rocids.size()==0){
        out << "no rocs connected?\n";
        return 0;
    }

    int wbc=fApi->_dut->getDAC(rocids[0], "wbc");
    
   
    // avoid the reset region in scans
    if(( (fSeq & 0x40)>0 ) && (tctmin < (wbc - fTRC) )){
        tctmin = wbc-fTRC;
    }
        
    
    unsigned int tct1=tct0;
    int n1=0;
    for(fTCT = tctmin; fTCT<=tctmax; fTCT++){
        int nhit=0;
        for(unsigned i=0; i<10; i++) nhit +=countHits();
        if (nhit>0){
            out << "tct=" <<  dec << fTCT << "  hits=" << nhit << "\n";
            flush(out);
            if (nhit>n1){ n1=nhit; tct1=fTCT;}
        }
    }
    
    if (n1>0){
        out << "tct set to " << tct1 << "\n";
        fTCT = tct1;
    }else{
        out << "no hits found, leaving tct at " << dec << tct0 <<"\n";
        fTCT = tct0;
    }
    return 0;
}



int CmdProc::tbmscan(const int nloop, const int ntrig, const int ftrigkhz){
    //string tbmtype = fApi->_dut->getTbmType(); //"tbm09c"

    uint8_t phasereg;
    int stat = tbmget("basee", TBMA, phasereg);
    if(stat>0){
        out << "error getting tbm delays from api \n";
    }
    uint8_t p400c= (phasereg>>2) & 7;
    uint8_t p160c= (phasereg>>5) & 7;
    
    uint8_t ntpreg;
    stat = tbmget("base0", TBMA, ntpreg);
    if(stat>0){
        out << "error getting base0 register from api \n";
    }
    
    int nroc=16;
    if( (ntpreg & 0x40) > 0 ){
        nroc=0;
    }

    // show the phase delays in time-ordered manner
    int p160_timed[8] = {0,1,2,3,4,5,6,7};
    sort_time(p160_timed, STEP160, RANGE160);
    out << "400\\160";
    for(uint8_t i = 0; i < sizeof(p160_timed); i++) {
        out << " " << i << " ";
    }
    out << "\n";
    int p400_timed[8] = {0,1,2,3,4,5,6,7};
    sort_time(p400_timed, STEP400, RANGE400);

    for(uint8_t p400=0; p400<8; p400++){
        int xor1[8] = {0,0,0,0,0,0,0,0};
        int xor2[8] = {0,0,0,0,0,0,0,0};
        out << "  " << (int) p400_timed[p400] << " :  ";
        for(uint8_t p160=0; p160<8; p160++){
            stat = tbmset("basee", TBMA, ((p160_timed[p160]&7)<<5)+((p400_timed[p400]&7)<<2));
            if(stat>0){
                out << "error setting delay  base E " << hex << (int) ((p160_timed[p160]<<5)+(p400_timed[p400]<<2)) << dec << "\n";
            }
            tbmset("base4", ALLTBMS, 0x10);// reset once after changing phases
            
            // waste a bit of time keeping the daq busy
            pxar::mDelay(10);
            int good= countGood(nloop, ntrig, ftrigkhz, nroc); //default 10 loops, 100 trigger, 10 kHz
            for(unsigned int i=0; i<8; i++){
                xor1[i] += good*fDeser400XOR1sum[i];
                xor2[i] += good*fDeser400XOR2sum[i];
            }
            char c=' ';
            if (good==nloop){ c='+';} 
            else if (good>(0.7*nloop)) { c='o' ;}
            else if (good>0) { c='.' ;}
            
            if((p160_timed[p160]==p160c)&&(p400_timed[p400]==p400c)){
               out << "(" << c << ")";
            }else{
               out << " " << c << " ";
            }
        }
        
        // only print phases when they can be real
        if( (ntpreg & 0x40) == 0 ){
            out << "    ";
            for(unsigned int i=0; i<8; i++){
                out << dec<< setw(5) << xor1[7-i]+xor2[7-i];
            }
        }
        out << "\n";
        flush(out);
    }
    tbmset("basee",TBMA,phasereg);
    return 0;
}



int CmdProc::test_timing(int nloop, int d160, int d400, int rocdelay, int htdelay, int tokdelay){
    int ftrigkhz = 100;
    int nroc=16;
    uint8_t value = ( (d160&0x7)<<5 ) + ( (d400&0x7 )<<2 );
    
    int stat = tbmset("basee", TBMA, value);
    if(stat>0){
        out << "error setting delay  base E " << hex << (int) value << dec << "\n";
    }
    
    if (rocdelay>=0){
        value = ( (tokdelay&0x1)<<7 ) + ( (htdelay&0x1)<<6 ) + ( (rocdelay&0x7)<<3 ) + (rocdelay&0x7);
        stat = tbmset("basea",ALLTBMS, value);
        if(stat>0){
            out << "error setting delay  base A " << hex << (int) value << dec << "\n";
        }
    }
    tbmset("base4", ALLTBMS, 0x10); // reset once after changing phases
    pxar::mDelay( 10 );
    return countGood(nloop, fNtrigTimingTest, ftrigkhz, nroc);
}







int CmdProc::test_tbmtiming(int nloop, const int tbmcores, const int daqchannels, 
        int d160, int d400, int rocdelay0, int rocdelay1, int htdelay, int tokdelay ){
    int ftrigkhz = 100;
    int nroc=0;

    if (d160>=0){
        uint8_t value = ( (d160&0x7)<<5 ) + ( (d400&0x7 )<<2 );
        int stat = tbmset("basee", tbmcores, value); // no mask needed, other bits are unused
        if(stat>0){
            out << "error setting delay  base E " << hex << (int) value << dec << "\n";
        }
    }
    
    if (rocdelay0>=0){
        nroc=16;
        uint8_t value = ( (tokdelay&0x1)<<7 ) + ( (htdelay&0x1)<<6 ) + ( (rocdelay1&0x7)<<3 ) + (rocdelay0&0x7);
        int stat = tbmset("basea",tbmcores, value);
        if(stat>0){
            out << "error setting delay  base A " << hex << (int) value << dec << "\n";
        }
    }
    
    
    tbmset("base4", ALLTBMS, 0x10); // reset once after changing phases
    pxar::mDelay( 10 );
    countGood(nloop, fNtrigTimingTest, ftrigkhz, nroc);
    
    // 
    int nch=0;
    int good=0;
    for(unsigned int i=0; i<fnDaqChannel; i++){
        if(daqchannels & (1<<i)){ nch++; good += fGoodLoops[ i ];}
    }
    if (nch>0){
        return int(good/nch);
    }else{
        // shouldn't be here
        return -983726;
    }
}




bool CmdProc::set_tbmtiming(int d160, int d400, int rocdelay[], int htdelay[], int tokdelay[], bool reset){
    
    bool ok = true;
    
    // single physical TBM for now (2 cores)
    uint8_t value=( (d160&0x7)<<5 ) + ( ( d400&0x7 )<<2);
    
    int stat = tbmset("basee", TBMA, value);
    if(stat>0){
        out << "error setting delay  base E " << hex << (int) value << dec << "\n";
        ok = false;
    }
    
       
    for(unsigned int core=0; core<fnTbmCore; core++){   
            value = 
                ( (tokdelay[core]&0x1)<<7 ) 
              + ( (htdelay[core]&0x1)<<6 )
              +   (rocdelay[2*core  ]&0x7)            // "port 0"
              + ( (rocdelay[2*core+1]&0x7)<<3 );      // "port 1"
            stat = tbmset("basea",1<<core, value);
            if(stat>0){
                out << "error setting delay  base A " << hex << (int) value << dec << "\n";
                ok = false;
            }
    }
    
    if(reset){
        tbmset("base4", ALLTBMS, 0x10); // reset once after changing phases
    }
    
    return ok;
}




void CmdProc::sort_time(int values[], double step, double range){
    // indirect sort according to time modulo range
    for(int i=0; i<8; i++){
        for(int j=0; j<7; j++){
            if(  fmod(values[j]*step,range) >  fmod(values[j+1]*step,range) ){
                int tmp=values[j]; values[j]=values[j+1]; values[j+1]=tmp;
            }
        }
    }
}

bool CmdProc::find_midpoint(int threshold, int data[], uint8_t & position, int & width){

    width=0;
    for(int i=0; i<8; i++){
        int w=0;
        int j=0;
        while((j<8) && (data[ (i+j) % 8 ]>=threshold) ){
            w++;
            j++;
        }
        if (w>width){
            width=w;
            position = int(i+w/2) % 8;
        }
    }
    
    return width>0;
}


bool CmdProc::find_midpoint(int threshold, double step, double range,  int data[], uint8_t & position, int & width){
    int m[8] = {0,1,2,3,4,5,6,7};
    sort_time(m, step, range);
    // now data[m[*]] is time-ordered
    
    width=0;
    for(int i=0; i<8; i++){
        int w=0;
        while( (w<8) && (data[ m[(i+w) % 8] ]>=threshold) ){
            w++;
        }
        if (w>width){
            width=w;
            position = m[int(i+w/2) % 8];
        }
    }
    return width>0;
}


int CmdProc::find_timing(int npass){
    // npass is the minimal number of passes
    
    string tbmtype = fApi->_dut->getTbmType();
    if (! ((tbmtype=="tbm09c")||(tbmtype=="tbm08c")||(tbmtype=="tbm10c")
	    || (tbmtype == "tbm08d")  || (tbmtype == "tbm10d")) ){
        out << "This only works for TBM08c/08d/09c/10c/10d! \n";
    }

    uint8_t register_0=0;
    uint8_t register_e=0;
    uint8_t register_a=0;
    tbmget("base0", TBMA, register_0);
    tbmget("basee", TBMA, register_e);
    tbmget("basea", TBMA, register_a);
    uint8_t d400= (register_e >> 2) & 0x7;
    uint8_t d160= (register_e >> 5) & 0x7;
    int tokendelay =(register_a >> 7) & 0x1;
    int htdelay =   (register_a >> 6) & 0x1;
    int rocdelay =  (register_a)&7;
    

    int nloop=10;
    
    // disable token pass
    tbmsetbit("base0",ALLTBMS , 6, 1);
    // diagonal scan to find something that works
    int nmax=0;
    for(uint8_t m=0; m<8; m++){
        int nvalid = test_timing(nloop, m, m);
        if(verbose) cout << "diag scan " << (int) m << "  valid = " << nvalid << "/ " << nloop << endl;
        if (nvalid>nmax){
            d400 = m; 
            d160 = m; 
            nmax = nvalid;
        }
        flush(out);
    }
    if (nmax==0){
        out << " no working phases found ";
        tbmset("base0",ALLTBMS ,register_0);
        tbmset("basee",ALLTBMS ,register_e);
        return 0;
    }
    if (verbose) cout << "diag scan result = " << (int) d400 << endl;
    
    for(int pass=0; pass<3; pass++){
        
        if (verbose) cout << " pass " << pass << endl;
         // scan 160 MHz @ selected position
        int test160[8]={0,0,0,0,0,0,0,0};
        for (uint8_t m=0; m<8; m++){
            if (pass==0){
                test160[m] = test_timing(nloop, m, d400);
            }else{
                test160[m] = test_timing(nloop, m, d400, rocdelay, htdelay, tokendelay);
            }
        }
        
        int w160=0;
        if (! find_midpoint(nloop, STEP160, RANGE160, test160, d160, w160)){
            out << "160 MHz scan failed  in pass " << pass << "  d400=" << (int) d400 << " " ;
            for(unsigned int i=0; i<8; i++){ out << " " << dec << (int) test160[i];}
            out << "\n";
            tbmset("base0",ALLTBMS ,register_0);
            tbmset("basee",ALLTBMS ,register_e);
            return 0;
        }
        if(w160==8){
            d160=0; // anything goes, 0 often seems to be ok
        }
        out << "160 MHz set to " << dec << (int) d160 << "  width=" << (int) w160 << "\n";
        flush(out);
        
        
        // scan 400 MHz @ selected position
        int test400[8]={0,0,0,0,0,0,0,0};
        for (uint8_t m=0; m<8; m++){
            if (pass==0){
                test400[m] = test_timing(nloop, d160, m);
            }else{
                test400[m] = test_timing(nloop, d160, m, rocdelay, htdelay, tokendelay);
            }
        }
        
        int w400=0;
        if (! find_midpoint(nloop, STEP400, RANGE400, test400, d400, w400)){
            out << "400 MHz scan failed ";
            tbmset("base0",ALLTBMS, register_0);
            tbmset("basee",ALLTBMS, register_e);
            return 0;
        }
        out << "400 MHz set to " << dec << (int) d400 <<  "  width="<< (int) w400 << "\n";
        flush(out);
        
     
        // now enable trigger (again) and scan roc and header trailer delay
        if(pass==0) tbmsetbit("base0",ALLTBMS, 6,0);
        
        
        int wmax=0;
        int sumtestmax=0;
        for(uint8_t dtoken=0; dtoken<2; dtoken++){
            for(uint8_t dheader=0; dheader<2; dheader++){
                int test[8]={0,0,0,0,0,0,0,0};  // for midpoint search
                int sumtest = 0;                // fallback, maxmimum search
                uint8_t portmax= 0;
                for(uint8_t dport=0; dport<8; dport++){
                    test[dport] = test_timing(nloop, d160, d400, dport, dheader, dtoken);
                    sumtest += test[dport];
                    if (test[dport]>test[portmax]) portmax=dport;
                    if(verbose) {cout << (int) d160 << "," << (int) d400 << "," << (int)dport << "," << (int)dheader << "," << (int)dtoken << " -> " <<(int)test[dport] << endl;}
                    out << (int) d160 << "," << (int) d400 << "," << (int)dport << "," << (int)dheader << "," << (int)dtoken << " -> " <<(int)test[dport] << endl;
                    flush(out);
                }
                int w=0;
                uint8_t d=0;
                if(find_midpoint(nloop, 1.0, 6.25, test, d, w)){
                    if( (w>wmax) || ( (w>0) && (w==wmax) && (dheader==dtoken)) ){
                        wmax=w; 
                        tokendelay = dtoken;
                        htdelay = dheader;
                        rocdelay = d;
                    }
                }else{
                    // mid point search failed, fall-back method:
                    if ( (pass==0) && (wmax==0) && (sumtest>sumtestmax) ){
                        tokendelay = dtoken;
                        htdelay = dheader;
                        rocdelay = portmax;
                    }
                    if (verbose && (pass==0)){
                        cout << "no midpoint " << (int) dtoken << " " << (int) dheader << "  : ";
                        for(unsigned int i=0; i<8; i++){cout << " " << test[i];}
                        cout << endl; 
                    }
                }
            }
        }
        
        out << "selecting " << dec << (int) d160 << " " << (int) d400 
            << " " << (int) rocdelay
            << " " << (int) htdelay
            << " " << (int) tokendelay
            << "   width = " << wmax
            << "   (160 400 rocs h/t token)\n";
        flush(out);

        
        int nloop2=fNtrigTimingTest;
        int result=test_timing(nloop2, d160, d400, rocdelay, htdelay, tokendelay);
        out << "result =  " << dec<<  result << " / " << nloop2 <<" \n" ;
        flush(out);
        
        if (result==nloop2) {
             // restore base0 (token pass)
            tbmset("base0", ALLTBMS, register_0);
            if(pass>=npass-1){
                out << "successful, done. \n";
               return 0;
            }else{
                out << "pass " << pass << " successful, continuing\n";
            }
        }else{
            out << "pass " << pass <<" failed, retrying \n";
        }
        flush(out);
    }

    
    out << "failed to find timings, sorry\n";
    // restore initial state
    tbmset("base0", ALLTBMS, register_0);
    tbmset("basea", ALLTBMS, register_a);
    tbmset("basee", TBMA, register_e);

    return 0;
}




int CmdProc::find_tbmtiming(int npass){
    /* find_timing for L1 modules
     * we have two tbms (4 cores) each core serves two channels
     * but only one port per core is used (for four channels)
     * tbm   tbmcore    api core    daq channels
     * 
     */
    
    string tbmtype = fApi->_dut->getTbmType();
    if (! ((tbmtype=="tbm09c")||(tbmtype=="tbm08c")||(tbmtype=="tbm10c")) ){
        out << "This only works for TBM08c/09c/10c! \n";
    }

    // keep the original values to restore in case of failure

    vector<TBMDelays> d;
    for(unsigned int tbm=0; tbm<fnTbm; tbm++){
        d.push_back( tbmgetDelays( tbm ) );
    }
    
    int nloop=10;
    
    // tbm here denotes physical tbms, not cores 
    for(uint8_t tbm=0; tbm<fnTbm; tbm++){
        
        // disable all tokens, only enable what we are trying to adjust
        tbmsetbit("base0", ALLTBMS , 6, 1); 
        
        int tbms = (1<<(2*tbm)); // mask argument for tbmset... commands
        uint8_t d400 = d[tbm].d400;
        uint8_t d160 = d[tbm].d160;
        int htdelay = d[tbm].htdelay[0]; 
        int rocdelay = d[tbm].rocdelay[0];
        int tokendelay = d[tbm].tokendelay[0];
        bool success=false;
        
        // diagonal scan to find something that works
        int nmax=0;
        for(uint8_t m=0; m<8; m++){
            int nvalid = test_tbmtiming( nloop, tbms, fTbmChannels[tbm], m, m );
            if(verbose) cout << "TBM " << (int) tbm << "diag scan " << (int) m << "  valid = " << nvalid << "/ " << nloop << endl;
            if (nvalid>nmax){
                d400 = m; 
                d160 = m; 
                nmax = nvalid;
            }
            flush(out);
        }
        if (nmax==0){
            out << " no working phases found  tbm " << (int) tbm;
            tbmsetDelays( d[tbm], tbm );
            break;
        }
        
        if (verbose) cout << "diag scan result = " << (int) d400 << endl;
        
        
        tbms = (3<<(2*tbm));  // from now on we mean both cores
        
        for(int pass=0; pass<3; pass++){
            
            if (verbose) cout << " pass " << pass << endl;
             // scan 160 MHz @ selected position
            int test160[8]={0,0,0,0,0,0,0,0};
            for (uint8_t m=0; m<8; m++){
                if (pass==0){
                    test160[m] = test_tbmtiming(nloop, tbms, fTbmChannels[tbm], m, d400 );
                }else{
                    test160[m] = test_tbmtiming(nloop, tbms, fTbmChannels[tbm], m, d400, rocdelay, rocdelay, htdelay, tokendelay );
                }
            }
            
            int w160=0;
            if (! find_midpoint(nloop, STEP160, RANGE160, test160, d160, w160)){
                out << "160 MHz scan failed  in pass " << pass << "  d400=" << (int) d400 << " " ;
                for(unsigned int i=0; i<8; i++){ out << " " << dec << (int) test160[i];}
                out << "\n";
                pass=99;
                break;
            }
            if(w160==8){
                d160=0; // anything goes, 0 often seems to be ok
            }
            out << "tbm " << (int) tbm << ": 160 MHz set to " << dec << (int) d160 << "  width=" << (int) w160 << "\n";
            flush(out);
            
            
            // scan 400 MHz @ selected position
            int test400[8]={0,0,0,0,0,0,0,0};
            for (uint8_t m=0; m<8; m++){
                if (pass==0){
                    test400[m] = test_tbmtiming(nloop, tbms, fTbmChannels[tbm], d160, m);
                }else{
                    test400[m] = test_tbmtiming(nloop, tbms, fTbmChannels[tbm], d160, m, rocdelay, rocdelay, htdelay, tokendelay);
                }
            }
            
            int w400=0;
            if (! find_midpoint(nloop, STEP400, RANGE400, test400, d400, w400)){
                out << "400 MHz scan failed ";
                pass=99;
                break;
            }
            out << "tbm " << (int) tbm << ": 400 MHz set to " << dec << (int) d400 <<  "  width="<< (int) w400 << "\n";
            flush(out);
            
         
            // now enable trigger (again) and scan roc and header trailer delay
            if(pass==0) tbmsetbit("base0",tbms, 6,0);
            
            
            int wmax=0;
            int sumtestmax=0;
            for(uint8_t dtoken=0; dtoken<2; dtoken++){
                for(uint8_t dheader=0; dheader<2; dheader++){
                    int test[8]={0,0,0,0,0,0,0,0};  // for midpoint search
                    int sumtest = 0;                // fallback, maxmimum search
                    uint8_t portmax= 0;
                    for(uint8_t dport=0; dport<8; dport++){
                        test[dport] = test_tbmtiming(nloop, tbms, fTbmChannels[tbm], d160, d400, dport, dport, dheader, dtoken);
                        sumtest += test[dport];
                        if (test[dport]>test[portmax]) portmax=dport;
                        if(verbose) {cout << (int) d160 << "," << (int) d400 << "," << (int)dport << "," << (int)dheader << "," << (int)dtoken << " -> " <<(int)test[dport] << endl;}
                        out << "tbm " << (int) tbm  << " : "<< (int) d160 << "," << (int) d400 << "," << (int)dport << "," << (int)dheader << "," << (int)dtoken << " -> " <<(int)test[dport] << endl;
                        flush(out);
                    }
                    int w=0;
                    uint8_t d=0;
                    if(find_midpoint(nloop, 1.0, 6.25, test, d, w)){
                        if( (w>wmax) || ( (w>0) && (w==wmax) && (dheader==dtoken)) ){
                            wmax=w; 
                            tokendelay = dtoken;
                            htdelay = dheader;
                            rocdelay = d;
                        }
                    }else{
                        // mid point search failed, fall-back method:
                        if ( (pass==0) && (wmax==0) && (sumtest>sumtestmax) ){
                            tokendelay = dtoken;
                            htdelay = dheader;
                            rocdelay = portmax;
                        }
                        if (verbose && (pass==0)){
                            cout << "no midpoint " << (int) dtoken << " " << (int) dheader << "  : ";
                            for(unsigned int i=0; i<8; i++){cout << " " << test[i];}
                            cout << endl; 
                        }
                    }
                }
            }
            
            out << "tbm " << (int) tbm 
                << ": selecting " << dec << (int) d160 << " " << (int) d400 
                << " " << (int) rocdelay
                << " " << (int) htdelay
                << " " << (int) tokendelay
                << "   width = " << wmax
                << "   (160 400 rocs h/t token)\n";
            flush(out);

            
            int nloop2=fNtrigTimingTest;
            int result=test_tbmtiming(nloop2, tbms, fTbmChannels[tbm], d160, d400, rocdelay, rocdelay, htdelay, tokendelay);
            out << "tbm " << (int) tbm << ": result =  " << dec<<  result << " / " << nloop2 <<" \n" ;
            flush(out);
            
            if (result==nloop2) {
                 // restore base0 (token pass)
                if(pass>=npass-1){
                     success = true;
                }else{
                    out << "pass " << pass << " successful, continuing\n";
                }
            }else{
                success = false;
                out << "pass " << pass <<" failed, retrying \n";
            }
            flush(out);
        }
        
        if (success){
                   out << "tbm " <<(int) tbm << " successful, done. \n";
        }else{
            out << "tbm" << (int) tbm << " failed, restoring \n";
            tbmsetDelays(d[tbm], tbm);          
        }
    }

    // restore token pass bits to what they were before we started
    for(unsigned int tbm=0; tbm<fnTbm; tbm++){
        tbmsetDelaysReg0(d[tbm], tbm);
    }
    
    printTbmPhases();
    
    return 0;
}


int CmdProc::printTbmPhases(){
    uint8_t phases, htdly;
    out << "tbm  160 MHz  400 MHz    port 1 0   TOK H/T\n";
    for(uint8_t core=0; core < fApi->_dut->getNEnabledTbms(); core++){

        if (fApi->_dut->getNEnabledTbms()>2){
            out << dec << (int) core/2 << (uint8_t)(65+(core%2)); //0A,0B,1A,1B
        }else{
            out <<" "<< (uint8_t)(65+(core%2));  //A or B
        }

        if( (core % 2)==0 ){
            tbmget("basee",  1<<core, phases);
            out << "      "  << dec << (int) ( (phases >> 5) & 7);
            out << "      "  << dec << (int) ( (phases >> 2) & 7);
        }else{
            out << "              ";
        }
        out << "      ";
        
        tbmget("basea", 1<< core , htdly);
        //out << "    " << (int) (htdly>>7) << "   " << (int) ((htdly>>6) &1) << "        " << (int) ((htdly>>3) &7)<<" " << (int) (htdly &7) << "\n";
        out  << "        " << (int) ((htdly>>3) &7)<<" " << (int) (htdly &7) 
            << "    " << (int) (htdly>>7) << "   " << (int) ((htdly>>6) &1) << "\n";
    }
    return 0;
}    

    
TBMDelays CmdProc::tbmgetDelays(uint8_t tbm){
    // get all delay settings of a single physical TBM into a TBMDelays object
    TBMDelays d;
    for(uint8_t core=0; core<fnCoresPerTBM; core++){
       uint8_t apicore = fnCoresPerTBM*tbm + core;
       if (core%2==0){
            tbmget("basee", 1<<apicore, d.register_e[core]);
            d.d400= (d.register_e[core] >> 2) & 0x7;
            d.d160= (d.register_e[core] >> 5) & 0x7;
        }
        tbmget("base0", 1<<apicore, d.register_0[core]);
        tbmget("basea", 1<<apicore, d.register_a[core]);
        d.tokendelay[core]  = (d.register_a[core] >> 7) & 0x1;
        d.htdelay[core]     = (d.register_a[core] >> 6) & 0x1;
        d.rocdelay[2*core  ]= (d.register_a[core]     ) & 7;  // port 0
        d.rocdelay[2*core+1]= (d.register_a[core] >>3 ) & 7;  // port 1
    }
    return d;
}


int CmdProc::tbmsetDelays(TBMDelays & d, uint8_t tbm){
    // set all delay settings from a TBMDelays
    for(uint8_t core=0; core<fnCoresPerTBM; core++){
       uint8_t apicore = fnCoresPerTBM*tbm + core;
       if (core%2==0){
            tbmset("basee", 1<<apicore, d.register_e[core]);
        }
        tbmset("base0", 1<<apicore, d.register_0[core]);
        tbmset("basea", 1<<apicore, d.register_a[core]);
    }
    return 0;
}

int CmdProc::tbmsetDelaysReg0(TBMDelays & d, uint8_t tbm){
    // set all delay settings from a TBMDelays
    for(uint8_t core=0; core<fnCoresPerTBM; core++){
       uint8_t apicore = fnCoresPerTBM*tbm + core;
        tbmset("base0", 1<<apicore, d.register_0[core]);
    }
    return 0;
}



int CmdProc::post_timing(int verbosity){
    /* tweak port delays
     * header/trailer /token bits exist once per tbm core
     * => for each core:
     *    vary header/trailer/token bits
     *      vary the two rocdelays of that core
     *       independently, in principle 8*8 for each h/t setting ....
     *       can try to do it iteratively instead
     *      
     *  tbm08           : two delays for one daqchannel
     *  tbm09 and tbm10 : exactly one daqchannel for each rocdelay
     * 
     */
    
    // keep the original values to restore in case of failure

    vector<TBMDelays> dTBM;
    for(unsigned int tbm=0; tbm<fnTbm; tbm++){
        dTBM.push_back( tbmgetDelays( tbm ) );
    }
    
    int nloop=10;
 
    out << "tbm    port 1 0   TOK H/T" << endl;
    for(unsigned int core=0; core<fnTbmCore; core++){
        
        int tbm = core / 2;
        int tbmcore = core % 2;
        int dports[2]={0,0};
        dports[0] = dTBM[tbm].rocdelay[tbmcore*2];
        dports[1] = dTBM[tbm].rocdelay[tbmcore*2+1];
        int coremask = 1<< core;
        
        // loop over header/trailer and token delay, make them the same for now

        int wmax=0;  // largest minimum of the two ports
        int dhttmax=0;
        int dportmax[2] = {0,0};
        
        for(int dhtt=0; dhtt<2; dhtt++){
            int dheader = dhtt;
            int dtoken  = dhtt;
            
            int wmin =0;
            int d[2]={0,0}; // best values for this dhtt
            int w[2]={0,0}; 
            dports[0] = dTBM[tbm].rocdelay[tbmcore*2];  //transient
            dports[1] = dTBM[tbm].rocdelay[tbmcore*2+1];
           
            for(unsigned int iter=0; iter<3; iter++){
                
                for(unsigned int port=0; port<2; port++){
                    
                    int ch;
                    if(fnTbmCore==4){ 
                        ch = ((2*core+port)+4) % 8;   // layer1
                    }else if( (fnTbmCore==2)&&(!tbm08())){
                        ch = 2*core + port;           // layer 2
                    }else{
                        ch = core;                    // layer 3/4  (untested)
                    }
                    int channelmask = (1<<ch);
                             
                    int dportsave=dports[port];
                    // scan this roc delay, find midpoint and width
                    int test[8]={0,0,0,0,0,0,0,0};  
                    for(uint8_t dport=0; dport<8; dport++){
                        dports[ port ] = dport;
                        test[dport] = test_tbmtiming(nloop, coremask, channelmask, -1, -1, 
                            dports[0], dports[1], dheader, dtoken);
                        cout << hex << coremask << ": " << dec << dports[0] << " " << dports[1] 
                             << " " << dheader << "  => " << test[dport] << endl;
                        
                    }

                    uint8_t dmid=0;
                    int wmid;
                    if(find_midpoint(nloop, 1.0, 6.25, test, dmid, wmid)){
                        if( (iter==0) || (min( wmid, w[(port+1)%2] ) > min( w[0],w[1] )) ) {
                            dports[port] = dmid; // use this value for the next iteration
                            w[port] = wmid;
                        }else{
                            dports[port]=dportsave;
                            w[port]=0;
                        }
                    }
                    if(verbosity>0){
                        out << dec << iter << ")" << (int) core/2 << (uint8_t)(65+(core%2)) << "." << (int) port  << "  "
                         << dports[1] << " " << dports[0] << "  " << dheader << "   " << dtoken << " " 
                        << "  width= " << wmid <<  "   wmin= " << wmin  << "   wmax=" << wmax 
                        << "  w0= " << w[0] << "  w1=" << w[1] <<  endl;
                        flush(out);
                        cout << dec << iter << ")" << (int) core/2 << (uint8_t)(65+(core%2)) << "." << (int) port  << "  "
                         << dports[1] << " " << dports[0] << "  " << dheader << "   " << dtoken << " " 
                        << "  width= " << wmid <<  "   wmin= " << wmin  << "   wmax=" << wmax 
                        << "  w0= " << w[0] << "  w1=" << w[1] <<  endl;
                    }
                }
                
                if( min(w[0],w[1]) > wmin){
                    // this iteration resultet in an improvement
                    d[0] = dports[0];
                    d[1] = dports[1];
                    wmin = min(w[0],w[1]);
                }else{
                    // no improvement, stop
                    break;
                }
            } // iteration 
            
            //end of rocdelay scans
            
            if (wmin>wmax){
                dportmax[0] = d[0];
                dportmax[1] = d[1];
                dhttmax = dhtt;
                wmax = wmin;
            }
            

        } // next htt
        
        // set the selected values
        int dtoken=dhttmax;
        int dheader=dhttmax;
        tbmset("basea", coremask, (dtoken<<7) + (dheader<<6) + (dportmax[1]<<3) + (dportmax[0])) ;
        dTBM[tbm] = tbmgetDelays( tbm );

        // done with this core
        out << "  "<< dec << (int) core/2 << (uint8_t)(65+(core%2)) << "        " 
            << dportmax[1] << " " << dportmax[0]<<  "    " << dheader << "   " << dtoken << " " 
            << "  width=" << wmax << endl;
        flush(out);
            
    } 
    // done with all cores
       
           
    
    return 0;
}




int CmdProc::rawscan(int level){
    uint8_t phasereg;
    int stat = tbmget("basee", TBMA, phasereg);
    if(stat>0){
        out << "error getting tbm delays from api \n";
    }
    
    uint8_t ntpreg;
    stat = tbmget("base0", TBMA, ntpreg);
    if(stat>0){
        out << "error getting base0 register from api \n";
    }
   
    out << "400\\160 0  1  2  3  4  5  6  7\n";
    for(uint8_t p400=0; p400<8; p400++){
        out << "  " << (int) p400 << " :  ";
        for(uint8_t p160=0; p160<8; p160++){
            stat = tbmset("basee", TBMA, ((p160&7)<<5)+((p400&7)<<2));
            if(stat>0){
                out << "error setting delay  base E " << hex << ((p160<<5)+(p400<<2)) << dec << "\n";
            }
            tbmset("base4", ALLTBMS, 0x10);// reset once after changing phases
            
            int stat = runDaqRaw(fBuf, 100, 10, 0, true);
            
            int n=0;
            if (stat==0){
                if (level==0){
                    n = fBuf.size();
                }else{
                    for(unsigned int i=0; i<fBuf.size(); i++){
                        if ((fBuf[i]&0xE000)==0xA000){
                            n+=1;
                        }
                    }
                }
            }
            
            out << (dec) << setw(5) << n ;
        }
        

        out << "\n";
    }
    tbmset("basee",TBMA ,phasereg);
    return 0;
}


int CmdProc::rocscan(){
    uint8_t phasereg0, phasereg1;
    int stat = tbmget("basea", TBMA, phasereg0) + tbmget("basea", TBMB, phasereg1);
    if(stat>0){
        out << "failed to get tbm registers from api\n";
    }
   
    uint8_t thbits0= (phasereg0 & 0xC0);
    //uint8_t thbits1= (phasereg1 & 0xC0);

    out << "token     0   |   1   |\n";  
    out << "h/t    0  | 1 | 0 | 1 |\n" ;
    out << "rocs:-----------------|\n";
    for(uint8_t dly=0; dly<8; dly++){
        out << "  " << dec << (int) dly << " :  ";
        for(uint8_t th=0; th<4; th++){
            tbmset("basea", TBMA, (th<<6) | (dly<<3) | (dly));
            tbmset("basea", TBMB, (th<<6) | (dly<<3) | (dly));
            
            int good=0; // 
            for(unsigned int k=0; k<10; k++){
                good += ( (countErrors(100,1)==0) ? 1 : 0);
            }
            char c=' ';
            if (good==10){ c='+';} 
            else if (good>7) { c='o' ;}
            else if (good>0) { c='.' ;}

            if( (dly==(phasereg0&0x7)) && (th==(thbits0>>6)) ){
               out << "(" << c << ")|";
            }else{
               out << " " << c << " |";
            }
            
        }
        out << "\n";
    }
    tbmset("basea",TBMA,phasereg0);
    tbmset("basea",TBMB,phasereg1);
    return 0;
}



int CmdProc::levelscan(){
    
    ConfigParameters *p = fPixSetup->getConfigParameters();
    if (fSigdelaysSetup.size()==0){
        fSigdelaysSetup = p->getTbSigDelays();
        fSigdelays = p->getTbSigDelays();
    }
    int l0=0;
    for(size_t i=0; i<fSigdelays.size(); i++){
        if (fSigdelays[i].first=="level"){ l0=fSigdelays[i].second;}
    }
    int nroc=16;

    for(unsigned int level=3; level<16; level++){
        
        setTestboardDelay("level", level); 
        if (fFW35) tbmset("base4", ALLTBMS, 0x10); // temp fix, avoid event counter reaching 255
        
        int good = countGood(100,1,0,nroc);
    
        out << dec << setw(3) << level << " : ";
        out << setw(5) << good << "  ";
        
        for(unsigned int i=0; i<8; i++){
            out << dec<< setw(4) << fDeser400XOR1sum[7-i];
        }
        out << "\n";

    }
    
    setTestboardDelay("level",l0);
    return 0;
}

int CmdProc::setTestboardPower(string name, uint16_t value){
    std::vector<std::pair<std::string,double> > power_settings;
    power_settings.push_back(make_pair(name, 0.001*float(value)));
    try{
        fApi->setTestboardPower(power_settings);
        return 0;
    }catch(pxar::InvalidConfig){
        out << "invalid config \n";
        return 1;
    }
    
}

int CmdProc::setTestboardDelay(string name, uint8_t value){
    
    if (fSigdelays.size()==0){
		// maybe never initialized
		ConfigParameters *p = fPixSetup->getConfigParameters();
        fSigdelaysSetup = p->getTbSigDelays();
        fSigdelays = p->getTbSigDelays();
   }else{
        // FIXME check whether gui values have changed ?????
   }
 

    for(size_t i=0; i<fSigdelays.size(); i++){
        if (fSigdelays[i].first==name){
            fSigdelays[i] =  std::make_pair(name, value);
        }
        if(verbose) cout << fSigdelays[i].first << " : " << (int) fSigdelays[i].second << endl; 
    }
    fApi->setTestboardDelays( fSigdelays );

    return 0;
}

uint8_t CmdProc::getTestboardDelay(string name){
    if (fSigdelays.size()==0){
		ConfigParameters *p = fPixSetup->getConfigParameters();
        fSigdelaysSetup = p->getTbSigDelays();
        fSigdelays = p->getTbSigDelays();
	}
    for(size_t i=0; i<fSigdelays.size(); i++){
		if (fSigdelays[i].first==name){
			return fSigdelays[i].second;
        }
	}
    out << "getTestboardDelay " << name << " not found \n";
    return 0;
}


int CmdProc::bursttest(int ntrig, int trigsep, int nburst, int caltrig, int loop){
        int stat = 0;
        while( ! ( (loop==0) || (stopped()) ) ) {
            burst(fBuf, ntrig, trigsep, nburst, caltrig);
        
            vector<DRecord > data;
            stat = getData(fBuf, data, 0);
        
            int n=0;
            for(unsigned int i=0; i<fBuf.size(); i++){
                if((fBuf[i]&0xe000)==0xa000) {
                    if(i>0) out << "\n";
                    out << dec << setfill(' ') << setw(4) << ++n << ": ";
                }
                out << setw(4) << setfill('0') << hex << fBuf[i] << " ";
            }
            out << setfill(' ') << endl;
            out << dec << stat << " errors\n";
            flush(out);
            loop--;
            if (stat>0) break;
        }
        return 0;
}

int CmdProc::adctest(const string signalName){

    // part 1 , acquire data : delay scan
   
    uint8_t gain = GAIN_1;
    uint8_t source = 1; // pg_sync
    uint8_t start  = 1;  // wait
    double scale=0.5;  // empirical for gain 1 ("+"-"-"), note that
    // a 50 Ohm terminated oscilloscope shows only half of that
    if ( signalName=="sda"){
        source = 2;        // trigger on sda
        start  = 7;
    }else if ( signalName=="rda"){
        source = 2;        // trigger on sda, hal generates some dummy i2c traffic
        start  = 17;
    }else if ( signalName=="ctr"){
        vector< pair<string, uint8_t> > pgsetup;
        pgsetup.push_back( make_pair("sync", 20 ) );
        // give the adc something to look at
        pgsetup.push_back( make_pair("resr", 20 ) );
        pgsetup.push_back( make_pair("cal", 20 ) );
        pgsetup.push_back( make_pair("trg", 20 ) );
        pgsetup.push_back( make_pair("tok", 0 ) ); 
        fApi->setPatternGenerator(pgsetup);
        gain = GAIN_1;
        source = 1; // sync        
    }else{
        vector< pair<string, uint8_t> > pgsetup;
        // no trigger! Need the idle pattern for TBM output
        // real readout is too fast. For single ROCs a token is needed.
        pgsetup.push_back( make_pair("resr", 20 ) );
        pgsetup.push_back( make_pair("sync;tok", 0 ) ); 
        fApi->setPatternGenerator(pgsetup);
        gain = GAIN_1;
        source = 1; // sync
    }
    
    uint16_t nSample = 1024;
    unsigned int nDly = 20; // stepsize 1.25 ns
    vector<int> y(nDly * nSample);
    int ymin=0xffff;
    int ymax=-ymin;
    vector<int> yhist(2048);

    for(unsigned int dly=0; dly<nDly; dly++){
        
        //sigdelays.clear();
        if(source==1){
            setTestboardDelay("clk", dly);
            setTestboardDelay("ctr", dly);
            setTestboardDelay("sda", dly+15);
            setTestboardDelay("tin", dly+5);
        }else if(source==2){
            setTestboardDelay("sda", dly);
        }
            
        vector<uint16_t> data = fApi->daqADC(signalName, gain, nSample, source, start);
        
        if (data.size()<nSample) {
            cout << "Warning, data size = " << data.size() << endl;
        }else{
            for(unsigned int i=0; i<nSample; i++){
                int raw = data[i] & 0x0fff;
                if (raw & 0x0800) raw |= 0xfffff000;  // sign
                if (raw>ymax) ymax=raw;
                if (raw<ymin) ymin=raw;
                y[ nDly*i+ nDly-1-dly ]=raw;
                yhist[ raw/2 + 1024]++;
           }
       }

   }

    // restore delays, signals (modified by daqADC) and pg
    setTestboardDelay("all");
	pg_restore();
     
    if (ymin<ymax){
        int ylo=ymin/2;  for(; yhist[ylo+1024]<yhist[ylo+1+1024]; ylo++){}
        int yhi=ymax/2;  for(; yhist[yhi+1024]<yhist[yhi-1+1024]; yhi--){}
        out << setw(6) << signalName 
            << "  low = " <<  fixed << setw(6) << setprecision(1) << ylo*scale*2 << " mV"
            << "  high= " <<  fixed << setw(6) << setprecision(1) << yhi*scale*2 << " mV "
            << "  amplitude = "<<  fixed << setw(6) << setprecision(1) << (yhi-ylo)*scale*2 <<  " mVpp  (differential)\n";
    }else{
        out << "no signal found\n";
    }

    return 0;
}



int CmdProc::tbmreadword(uint8_t regId, int hubId, int ntry){
 /* tbm i2c readback, one byte from register regId
  * the higher four bits determine the tbm core: 0xF* or 0xE*
  * in case of multiple tbms, the proper rda line must have been 
  * chosen before
  * the hubId is only used for verification
  * repeat ntry times if it fails, default argument value = 3
  * 
  *   from the TBM documentation:
  *   The TBM returns:
  *   Start (TBM Register Address) (Data Byte) Stop (Hub/Port Address*)
  *           (* Hub/Port Address does not use the 10 bit encoding in return data)
  *       TBM Register Address is 10 bit encoded, i.e. the 5th and 10th bits 
  *	 are the inverse of the 4th and 9th bit, respectively:
  *	  (4-bit register msb) X (4-bit register lsb) X
  *
  * returns -1 when data was invalid
  */
  
    uint8_t gain = GAIN_1;
    uint8_t start  = 17;  // wait after sda
    
    while((--ntry) >= 0){
		// get the raw waveform
		uint16_t nSample = 100;
		vector<uint16_t> data = fApi->daqADC("rda", gain, nSample, regId, start);
			
		if (data.size()<nSample) {
			cout << "tbmreadword : Warning, data size = " << data.size() << endl;
			return -1;
		}
		
		// convert level to binary
		vector< int > b;
		vector< int > v;
		for(unsigned int i=0; i<nSample; i++){
			int raw = data[i] & 0x0fff;
			if (raw & 0x0800) raw |= 0xfffff000;  // sign
			v.push_back( raw );
			if (raw>0){ b.push_back(1);}else{b.push_back(0);}
		}
		
		// search for a valid sequence within the bitstream
		for(unsigned int istart=0; istart<b.size()-30; istart++){
			uint8_t S=0;	 // reflected address, lowest bit should be 1
			S  = (b[istart+1])<<7;
			S |= (b[istart+2])<<6;
			S |= (b[istart+3])<<5;
			S |= (b[istart+4])<<4;
			bool compS3 = (b[istart+4] == b[istart+5]);
			S |= (b[istart+6])<<3;
			S |= (b[istart+7])<<2;
			S |= (b[istart+8])<<1;
			S |= (b[istart+9]); // called RW  in the tbm doc
			bool compRW = (b[istart+9] == b[istart+10]); 
				
			uint8_t D=0;  // readback data
			D  = (b[istart+11])<<7;
			D |= (b[istart+12])<<6;
			D |= (b[istart+13])<<5;
			D |= (b[istart+14])<<4;
			bool compD4 = (b[istart+15]==1);  // for readback data is 255
			D |= (b[istart+16])<<3;
			D |= (b[istart+17])<<2;
			D |= (b[istart+18])<<1;
			D |= (b[istart+19]);
			bool compD0 = (b[istart+20]==1);  // for readback data is 255

			bool bstop = (abs( v[istart]-v[istart+21] ) > (0.5*abs(v[istart]+v[istart+21])));
			
			uint8_t H=0;  // hubId
			H  = b[istart+22] << 4;
			H |= b[istart+23] << 3;
			H |= b[istart+24] << 2;
			H |= b[istart+25] << 1;
			H |= b[istart+26];
				   
			uint8_t P=0; // port, =4 for tbm readback
			P  = b[istart+27] <<2;
			P |= b[istart+28] <<1;
			P |= b[istart+29];

			bool valid = (S==(regId | 1))  && (H==hubId) && (P==4)
			  &&  !compS3 && !compRW && !compD4 && !compD0 && bstop;
			

			if( verbose && (S==(regId | 1))  && (H==hubId) && (P==4) ){
				cout << "tbmread "   << istart << "  valid=" << valid 
					<< "  port=" <<  (int) P << "  hub=" << (int) H 
					<< "  address = 0x" << hex << (int) S  << " regid= " << (int) regId
					<< "  data= 0x" << hex <<  (int) D 
					<< "  S3 = " << compS3 << "   RW=" << compRW << " " << "  D4= " << compD4 << "  D0=" << compD0 
					<<   "  bstop=" << bstop 
					<< " start= " << dec << v[istart] << "  stop=" << v[istart+21] << " " << v[30] 
					 << endl;
			}   
			   
			if (valid ){
				return (int) D;
			}
		} 
	}
	return -1;
}


int CmdProc::tbmread(uint8_t regId, int hubId){
  /* tbm i2c readback :
     from the TBM documentation:
     The TBM returns:
     Start (TBM Register Address) (Data Byte) Stop (Hub/Port Address*)
             (* Hub/Port Address does not use the 10 bit encoding in return data)
         TBM Register Address is 10 bit encoded, i.e. the 5th and 10th bits 
	 are the inverse of the 4th and 9th bit, respectively:
	  (4-bit register msb) X (4-bit register lsb) X

     scans the rda delay and returns the first valid data encountered
  */

    uint8_t sda0 = getTestboardDelay("sda");
    unsigned int nDly = 20; // stepsize 1.25 ns

    for(unsigned int dly=0; dly<nDly; dly++){
        setTestboardDelay("sda", dly);
		int D = tbmreadword(regId, hubId);
		if (D >= 0){
			setTestboardDelay("sda",sda0);
			setTestboardDelay("all");
			return D;
		}
    }

    // restore delays, signals (modified by daqADC) and pg
    setTestboardDelay("sda",sda0);
    setTestboardDelay("all");
    return -1;
}



int CmdProc::tbmread(uint8_t regId, int hubId, bool scan, unsigned int & delay){
  /* tbm i2c readback :
   * either with fixed delay or scanning delay values (if scan==true)
  */
 
	//save sda, restore it afterwards
	uint8_t sda0 = getTestboardDelay("sda");

    if (! scan){  // fixed delay
		setTestboardDelay("sda", delay);
		int D = tbmreadword(regId, hubId);
		setTestboardDelay("sda",sda0);
		return D;
    }
		
	// scan
    unsigned int nDly = 20; // stepsize 1.25 ns
    vector<int> result;
    for(unsigned int dly = 0; dly < nDly; dly++){
        setTestboardDelay("sda", dly);
 		int D = tbmreadword(regId, hubId);
 		result.push_back(D);
	}
	
	// select a good delay value for repeated use
	int nbest = 0;
	int dlybest = 0;
    for(unsigned int dly=0; dly<nDly; dly++){
	    if(result.at(dly) >= 0){
	        int ngood =1;
	        for (unsigned int m=1; m < nDly; m++){
	           if (result.at((dly+m) % nDly) == result.at(dly)){
	              ngood += 1;
	           }else{
		            break;
	           }
		    }
			if (ngood > nbest){
			    nbest = ngood;
				dlybest = int(dly + ngood/2 ) % nDly;
		    }
		}
    }
    delay = dlybest; // passed by reference
	setTestboardDelay("sda",sda0);
	setTestboardDelay("all");
	return result.at(dlybest);
}




string CmdProc::tbmprint(uint8_t regId, int hubid){
	stringstream s;
	int value = tbmread(regId, hubid);
	if (value>=0){
		s<< "      0x" << (hex) << setfill('0') << setw(2) << value << setfill(' ');
	}else{
		s<< "       err";
	}
	return s.str();
}


int CmdProc::tbmreadback() {
    // read some registers of the TBMs.
    for(unsigned int i = 0; i < (fnTbmCore / 2); i++) {
        // in order to read back TBM registers for layer 1 modules
        // we have to use the analog switch of the module adapter
        if (fnTbmCore > 2) fApi->selectTbmRDA(i);
		string tbmtype = fApi->_dut->getTbmType();
		if (! ((tbmtype=="tbm09c")||(tbmtype=="tbm08c")||(tbmtype=="tbm10c")
            || (tbmtype=="tbm08d")||(tbmtype=="tbm10d") )){
			out << "This only works for TBM08c/08d/09c/10c/10d! \n";
		}

        int hubid = fApi->_dut->getEnabledTbms().at(i*2).hubid;
        out << "TBM " << (dec) << i << ", hubid: " << hubid << "\n" ;
        out << "               core A      core B \n";
        out << "Base + 1/0 " << tbmprint(0xe1, hubid) << "  " << tbmprint(0xf1, hubid) << "\n";
        out << "Base + 3/2 " << tbmprint(0xe3, hubid) << "  " << tbmprint(0xf3, hubid) << "\n";
        out << "Base + 9/8 " << tbmprint(0xe9, hubid) << "  " << tbmprint(0xf9, hubid) << "\n";
        out << "Base + B/A " << tbmprint(0xeb, hubid) << "  " << tbmprint(0xfb, hubid) << "\n";
        out << "Base + D/C " << tbmprint(0xed, hubid) << "  " << tbmprint(0xfd, hubid) << "\n";
		out << "Base + F/E " << tbmprint(0xef, hubid) << "  " << tbmprint(0xff, hubid) << "\n"; 
	    out << (dec) ;
    }
    return 0;
}



vector<vector<int>> CmdProc::tbmreadstack() {
    /*  read the stack of all tbm cores
     * returns one vector per core with 33 entries \
     * = 32 stack values  + the stack count (bits 0-5 of base+3)
     * 
     * in at least one module this has the unwanted side-effect that the
     * content of the 0xFC register in the first TBM changes
     * (=autoreset counter of tbm0)
     * This is not understood. It appears to happend during the delay scan.
     *      tbmread(0xf3, hubid, true,  delay);
     * 
     *  */
    
    // save the content of the base+0 registers
    uint8_t reg0[4];
    for(unsigned int core=0; core<fnTbmCore; core++){
      tbmget("base0", 1<<core, reg0[core]);
    }
    // stack readback mode :  Pause Readout, Ignore Incoming Triggers, Stack Readback Mode.
    tbmset("base0", ALLTBMS, 0x78);

    vector<vector<int>> data;
    for(unsigned int i = 0; i < (fnTbmCore / 2); i++) {
 
        int hubid = fApi->_dut->getEnabledTbms().at(i*2).hubid;
        	
		// select the proper RDA on the adapter (L1 only)
		if (fnTbmCore > 2) fApi->selectTbmRDA(i);  // for some reason needed here again

		// stack data vectors, 32*stack + count
		vector<int> stack_a(33,-1), stack_b(33,-1);

		// read the stack count readback register (Base+3)
		unsigned int delay = 0;
		stack_a[32] = tbmread(0xf3, hubid, true,  delay);
		stack_b[32] = tbmread(0xe3, hubid, false, delay);
		
		if (fnTbmCore > 2) fApi->selectTbmRDA(i);
		// read the stack data base+7 and base+5
		for(unsigned int k = 0; k<32; k++){
		    
			int stat_a = tbmread(0xe7, hubid, false, delay);
			int stat_b = tbmread(0xf7, hubid, false, delay);
			int evt_a  = tbmread(0xe5, hubid, false, delay);
			int evt_b  = tbmread(0xf5, hubid, false, delay);
					
			if( (stat_a >= 0) && (evt_a >=0 )){
				stack_a[k] = ( (stat_a << 8) + evt_a );
			}
			
			if( (stat_b >= 0) && (evt_b >=0 )){
				stack_b[k] = ( (stat_b << 8) + evt_b );
			}
		}
		
		data.push_back(stack_a);
		data.push_back(stack_b);

	}

	// revert base+0 registers
	for(unsigned int core=0; core<fnTbmCore; core++){
	  tbmset("base0", 1<<core, reg0[core]);
	}

    return data;
}


int CmdProc::printStack( const vector<vector<int> > stackdata, stringstream & outstream){
	
	unsigned int ncore = stackdata.size();
	stringstream s;

	
	s.str("");
	s << "core ";
	for(unsigned int n = 0; n< ncore; n++){
		if ( (n % 2) == 0) {
			s << "            " << dec << int(n/2) << " A    ";
		}else{
			s << "       " << dec << int(n/2) << " B      ";
		}
	}
	outstream << s.str() << "\n";
	
	s.str("");
	s << "count";
	for(unsigned int n = 0; n< ncore; n++){
		if ( (n%2) == 0 ){ s << "    ";}
		if (stackdata[n][32] >= 0) {
			s<< dec << setw(15) << (stackdata[n][32] & 0x3F) << setfill(' ');
		}else{
			s<< "       err";
		}
	}
	outstream << s.str() << "\n";
	
	for( unsigned int k=0; k<32; k++){
		s.str("");
		s << dec << setw(4) << k << ")";
		for (unsigned int n=0; n < ncore; n++){
			if ( ( n%2 ) == 0) {s << "    ";}
			int word = stackdata[n][k];
			if (word >= 0){
				s<< "   " << bitset<8>( (word >> 8) & 0xFF);
				s<< dec << setw(4) << (word & 0xFF) << setfill(' ');
			}else{
				s<< "            err";
			}
		}	
		outstream << s.str() << "\n";
	}
	return 0;
}



int CmdProc::monitorStack(int period_in_s, int number_of_periods, int run_number){
	vector<vector<int> > stackdata = tbmreadstack();
    printStack(stackdata, out);
    flush(out);
    
    timer t0;
    
    ofstream fout;
    if (run_number >  0){
 		stringstream out_filename;
		out_filename << "seu_run_" << dec << setw(6) << setfill('0') << run_number << setfill(' ') << ".log";
		fout.open( out_filename.str());
		if(fout){
			fout << "seu test, initial stack\n";
			stringstream s;
			printStack(stackdata, s);
			fout << s.str() << endl;
		}else{
			cerr << "unable to open " << out_filename << endl;
		}
	}
	    
    int n = 0;
    int ndiff_total = 0;
    bool aborted = false;
    while ((n< number_of_periods) && (not aborted) ){
        n++;
        timer t;
		while( (t.get() < (1000.*period_in_s)) && (not aborted)) {
			pxar::mDelay( 100 ); 
			aborted = stopped();
        }
        vector<vector<int> > newdata = tbmreadstack();
        
        // compare
		unsigned int ncore = stackdata.size();
		int ndiff = 0;
		for (unsigned int n=0; n < ncore; n++){
			for( unsigned int k=0; k<32; k++){
				if ((stackdata[n][k]>=0) && (newdata[n][k]>=0) && (!(stackdata[n][k] == newdata[n][k]))){
					// count bit flips
					int wxor = stackdata[n][k] ^ newdata[n][k];
					for(unsigned int b=0; b<16; b++){
						if (((wxor >> b) & 1) > 0){
							ndiff ++;
						}
					}
				}
			}
		}
		
		out << "loop nr " << dec << n << " timestamp= " << t0.get() << "  " << ndiff << " differences "<< endl;
		if (fout) {fout << "loop nr " << dec << n << " timestamp= " << t0.get() << "  " << ndiff << " differences "<< endl;}
		if (ndiff > 0){
			ndiff_total += ndiff;
			printStack(newdata, out);
			if (fout){
				stringstream s;
				printStack(newdata, s);
				fout << s.str() << endl;
			}
			stackdata = newdata;
		}
        flush(out);
	}
 
 
	out << "total number of differences " << dec << ndiff_total << " in " << 0.001*t0.get() << " seconds\n";
   if(fout){
		fout << "total number of differences " << dec << ndiff_total << " in " << 0.001*t0.get() << " seconds\n";
		fout.close();
    }

	return 0;
}


int CmdProc::pixDecodeRaw(int raw, int level){
    int ph(0), error(0),x(0),y(0);
    string s="";
    
    // direct copy from psi46test
    ph = (raw & 0x0f) + ((raw >> 1) & 0xf0);
    error = (raw & 0x10) ? 128 : 0;
    if((error==128)&&(level>0)){ s=", wrong stuffing bit";}

    if (fApi->_dut->getRocType() >= ROC_PROC600) {
        x = ((raw >> 17) & 0x07) + ((raw >> 18) & 0x38);
        y = ((raw >> 9) & 0x07) + ((raw >> 10) & 0x78);
    }
    else {
        int c1 = (raw >> 21) & 7;
        if (c1 >= 6) {
            error |= 16;
            s += ", illegal raw column data (msb)";
        };
        int c0 = (raw >> 18) & 7;
        if (c0 >= 6) {
            error |= 8;
            s += ", illegal raw column data (lsb)";
        };
        int c = c1 * 6 + c0;

        int r2 = (raw >> 15) & 7;
        if (r2 >= 6) {
            error |= 4;
            s += ", illegal raw row data (msb)";
        };
        int r1 = (raw >> 12) & 7;
        if (r1 >= 6) {
            error |= 2;
            s += ", illegal raw row data (nmsb)";
        };
        int r0 = (raw >> 9) & 7;
        if (r0 >= 6) {
            error |= 1;
            s += ", illegal raw row data (lsb)";
        };
        int r = (r2 * 6 + r1) * 6 + r0;

        y = 80 - r / 2;
        if ((unsigned int) y >= 80) {
            error |= 32;
            s += ", bad row";
        };
        x = 2 * c + (r & 1);
        if ((unsigned int) x >= 52) {
            error |= 64;
            s += ", bad column";
        };
    }if(level>0){
    out << "( " << dec << setw(2) << setfill(' ') << x
         << ", "<< dec << setw(2) << setfill(' ') << y 
         << ": "<< setw(3) << ph << ") ";
    }
    if((error>0)&&(level>0)){
        out << "error =" << error << " " << s;
    }
    return error;
}

int CmdProc::pixDecodeRaw(int raw, uint8_t & col, uint8_t & row, uint8_t & ph){
    int error(0),x(0),y(0);
    string s="";
    
    // direct copy from psi46test
    ph = (raw & 0x0f) + ((raw >> 1) & 0xf0);
    error = (raw & 0x10) ? 128 : 0;

    if (fApi->_dut->getRocType() >= ROC_PROC600)  {
        col = ((raw >> 17) & 0x07) + ((raw >> 18) & 0x38);
        row = ((raw >> 9) & 0x07) + ((raw >> 10) & 0x78);
    }
    else {
        int c1 = (raw >> 21) & 7;
        if (c1 >= 6) { error |= 16; };
        int c0 = (raw >> 18) & 7;
        if (c0 >= 6) { error |= 8; };
        int c = c1 * 6 + c0;

        int r2 = (raw >> 15) & 7;
        if (r2 >= 6) { error |= 4; };
        int r1 = (raw >> 12) & 7;
        if (r1 >= 6) { error |= 2; };
        int r0 = (raw >> 9) & 7;
        if (r0 >= 6) { error |= 1; };
        int r = (r2 * 6 + r1) * 6 + r0;

        y = 80 - r / 2;
        if ((unsigned int) y >= 80) { error |= 32; };
        x = 2 * c + (r & 1);
        if ((unsigned int) x >= 52) { error |= 64; };
        col = static_cast<uint8_t>(x);
        row = static_cast<uint8_t>(y);
    }
    return error;
}

int CmdProc::rawRocReadback(uint8_t  signal, std::vector<uint16_t> & values){
    /* readback bits in the roc header(s), if signal is not 0xff,
     * program the readback register first, otherwise read back whatever
     * the readback register has previously been set-up for.
     * The latter mode is useful for the 'last dac' readback, while
     * the former is useful for analog readbacks (triggers the ADC)
     */
     
    if (signal<0xff){
        if ( ! (fApi->setDAC("readback", signal))){
            out << "Warning ! May have failed to write to readback register\n";
        }
    }

    size_t nTBM = fApi->_dut->getNTbms();
    
    if (nTBM==0){
        pg_sequence( 3 ); // token+trigger
    }else{
        pg_sequence( 2 ); // trigger only
    }
    fApi->daqTriggerSource("pg_dir");
    fApi->daqStart(fBufsize, fPixelConfigNeeded);
    fApi->daqTrigger(100, fPeriod);
    try { fApi->daqGetRawEventBuffer(); }
    catch(pxar::DataNoEvent &) {}
    fApi->daqTrigger(16, fPeriod);
    std::vector<pxar::rawEvent> buf;
    try { buf = fApi->daqGetRawEventBuffer(); }
    catch(pxar::DataNoEvent &) {}
    fApi->daqStop(false);
    pg_restore();

    if(buf.size()<16){
        out << "only got " << buf.size() << " events instead of 16 !\n";
        return -1;
    }
    
    size_t nRoc = 0;
    values.clear();
    for(unsigned int i=0; i<16; i++){ values.push_back(0);}
    
    std::vector<uint16_t>  raw_values;
    raw_values.clear();
    
    vector<int> start;
    for(unsigned int i=0; i<buf.size(); i++){
        unsigned int iroc=0;
        for(unsigned int k=0; k<buf[i].data.size(); k++){
            uint16_t w = buf[i].data[k];
            // only look at roc headers (0x4...)
            if(    ( (nTBM == 0) && (k==0) ) 
                || ( (nTBM > 0 ) && ((w & 0xf000)== 0x4000)) ){
                uint8_t D = (w & 1);
                uint8_t S = (w >>1) & 1;
                if (iroc==raw_values.size()) { raw_values.push_back(0); start.push_back(-1); }
                raw_values[iroc] = ( raw_values[iroc] << 1 ) + D;
                if (S==1){
                    start[iroc]=i;
                }
                iroc++;
                if (verbose) {cout << (int) iroc << " "; }
            }
        }
        if(iroc>nRoc) nRoc= iroc;
        if(verbose) {cout << "  nroc=" << (int) nRoc << endl;}
    }
    
    
    if(nRoc == 0){
        
        out << "readback failed, no roc headers!\n";
        return -1;

    }else{
        
        int flags = 0;
        for(uint8_t iroc=0; iroc<nRoc; iroc++){
            uint8_t expected_rocid = rocIdFromReadoutPositionRaw( iroc );
            int value = (raw_values[iroc] >> (15-start[iroc]) ) | (raw_values[iroc]<<(start[iroc]+1));
            if(verbose){
                cout << "readback roc " << setw(2) << (int) iroc 
                << " expected id " << setw(2) << (int) expected_rocid
                << "   start=" << setw(2) << start[iroc]
                << "      raw = " <<  bitset<16>( values[iroc] ) 
                << "  aligned = " << bitset<16>(value) << endl;
            }
            if(start[iroc]>=0){
                values[ expected_rocid ] = value;  // store the aligned value
            }else{
                flags |= (1<<expected_rocid); // no startbit
            }
        }
        return flags;
    }
    
    return -1;
}

int CmdProc::readRocsAnalog(uint8_t  signal, double scale, std::string units){
    /* readback bits in the roc header(s), normalize to vbg
     * if signal must be a valid analog adc register (8..12)
     * program the readback register first, otherwise read back whatever
     * the readback register has previously been set-up for.
     * The latter mode is useful for the 'last dac' readback, while
     * the former is useful for analog readbacks (triggers the ADC)
     */
     
    if ((signal<8)||(signal>12)){
        out << "only meaningful for analog readback registers\n";
        return 1;

    }

    std::vector<uint16_t> vbg;
    int flags = rawRocReadback( 11, vbg);
    
    if ((flags==-1)||(vbg.size()==0)){
        out << "error reading vbg for normalization\n";
        return 2;
    }
    
    std::vector<uint16_t> values;
    flags = rawRocReadback( signal, values);
    if (flags==-1){
        out << "error reading values\n";
        return 3;
    }

    if (!(vbg.size()==values.size())){
        out << "values and reference readback inconsistent \n";
        return 4;
    }
    
    
    float xaverage=0;
    int average=0;
    int nvalid=0;
    for(uint8_t roc=0; roc<values.size(); roc++){
        
        uint8_t rocid= (values[roc] & 0xF000) >> 12;
        uint8_t cmd  = (values[roc] & 0x0F00) >>  8;
        uint8_t data = (values[roc] & 0x00FF);
        float x=0;
        if (vbg[roc]>0){
            x= float(data) * scale / (float(vbg[roc]&0xff) * 0.008 ) * 1.22;
        }
        
        out << dec << fixed << setfill(' ') << setw(2) << (int) roc
                <<  "(" << setw(2) << (int) rocid << ")" << ": " 
                << fixed << setw(3) << (int) data;
        if((scale>0) && (vbg[roc]>0)){
            out << " ~ " << fixed << setw(6)  << setprecision(3) << x 
                << " " << units;
        }
        if ((cmd == signal)||(signal==0xff)){
            out << "\n";
            average += data;
            xaverage += x;
            nvalid+=1;
        }else{
            out <<  "   readback inconsistent  " << (int)cmd << " <> " << (int) signal << "\n";
        }
            
        if( (flags & (1<<roc)) > 0){
            out << " error\n";
        }
    }   


    if((values.size()>1)&&(nvalid>0)){
        out << "average:"  << dec<<fixed << setw(3) << int(average/nvalid);
        if(scale>0){
            out << " ~ " << fixed << setw(6)  << setprecision(3) << xaverage/nvalid
                << " " << units;  
        }
        out << "\n";
    }

    return 0;
}



int CmdProc::readRocs(uint8_t  signal, double scale, std::string units){
    /* readback bits in the roc header(s), if signal is not 0xff,
     * program the readback register first, otherwise read back whatever
     * the readback register has previously been set-up for.
     * The latter mode is useful for the 'last dac' readback, while
     * the former is useful for analog readbacks (triggers the ADC)
     */
     
    std::vector<uint16_t> values;
    int flags = rawRocReadback(signal, values);
    if (flags==-1){
        out << "error reading back roc data\n";
    }else{
        int average=0;
        int nvalid=0;
        for(uint8_t roc=0; roc<values.size(); roc++){
            if( ((flags >> roc)&1)==0 ){
                uint8_t rocid= (values[roc] & 0xF000) >> 12;
                uint8_t cmd  = (values[roc] & 0x0F00) >>  8;
                uint8_t data = (values[roc] & 0x00FF);
                out << dec << fixed << setfill(' ') << setw(2) << (int) roc
                        <<  "(" << setw(2) << (int) rocid << ")" << ": " 
                        << fixed << setw(3) << (int) data;
                if(scale>0){
                    out << " ~ " << fixed << setw(6)  << setprecision(3) << data*scale 
                        << " " << units;
                }
                if ((roc==rocid)&&((cmd == signal)||(signal==0xff))){
                    out << "\n";
                    average += data;
                    nvalid+=1;
                }else{
                    out <<  "   readback inconsistent  " << (int)cmd << " <> " << (int) signal << "\n";
                }
            }else{
                 out << dec << fixed << setfill(' ') << setw(2) << (int) roc << " no startbit?\n";
            }   
        }
        
        if(nvalid>0){
            out << "average:"  << dec<<fixed << setw(3) << int(average/nvalid);
            if(scale>0){
                out << " ~ " << fixed << setw(6)  << setprecision(3) << float(average)/nvalid*scale
                    << " " << units;  
            }
            out << "\n";
        }
    }
 
    pg_restore();
    return 0;
}

int CmdProc::getBuffer(vector<uint16_t> & buf){
    for(size_t i=0; i<nDaqChannelMax; i++){
        fDeser400XOR[i]=0;
    }
    if (fGetBufMethod==1){
        buf.clear();
        vector<rawEvent> vre;
        try { vre = fApi->daqGetRawEventBuffer(); }
        catch(pxar::DataNoEvent &) {}
        
        for(unsigned int i=0; i<vre.size(); i++){
            for(unsigned int j=0; j<vre.at(i).GetSize(); j++){
                buf.push_back( vre.at(i)[j] );
            }
        }
    }else{
        buf.clear();
        try { buf  = fApi->daqGetBuffer(); }
        catch(pxar::DataNoEvent &) {}
    }
    return 0;
}



int CmdProc::setupDaq(int ntrig, int ftrigkhz, int verbosity){
    /* setup pattern generator and daq
     * always call restoreDaq before returning control to Pxar
     */
    
    // warn user when no data expected
    if( (ntrig>0) && (fApi->_dut->getNTbms()>0) && ((fSeq & 0x02 ) ==0 ) ){
        out << "The current sequence does not contain a trigger!\n";
    }else if( (fApi->_dut->getNTbms()==0) && ((fSeq & 0x01 ) ==0 ) ){
        out <<"The current sequence does not contain a token!\n";
    }
    
    int length=0;
    if((ntrig==0) || (ntrig==1) || (ftrigkhz==0)){
        length=fBufsize/8;
    }else{
        length = 40000 / ftrigkhz;
    }
    
    pg_sequence( fSeq, length ); // set up the pattern generator
    fApi->daqTriggerSource("pg_dir");
    bool stat = fApi->daqStart(fBufsize, fPixelConfigNeeded);
    
    for(unsigned int i=0; i<8; i++){ fDeser400XOR1sum[i]=fDeser400XOR2sum[i]=0;}

    fPixelConfigNeeded = false;
    if (! stat ){
        if(verbosity>0){ out << "something wrong with daqstart !!!" << endl;}
        return 2;
    }
    
    if(verbose) out << "runDaq  " << dec << ftrigkhz  << "   " << length << "  " << fPeriod << endl;
    if(verbose) cout << "runDaq  " << dec << ftrigkhz  << "   " << length << "  " << fPeriod << endl;
    int leff = 40* int(length / 40);  // emulate testboards cDelay
    if((leff<length)&&(ntrig>1)){
        out << "period will be truncated to " << dec<< leff << " BC  = " << int(40000/leff) << " kHz !!" << endl;
    }
    return 0;
    
}


int CmdProc::restoreDaq(int verbosity){
    bool stat = fApi->daqStop(false);
    if( (! stat ) && (verbosity>0) ){
        out  << "something wrong with daqstop !" <<endl;
    }
    pg_restore(); // undo any changes
    return 0;
}


int CmdProc::runDaqRaw(vector<uint16_t> & buf, int ntrig, int ftrigkhz, int verbosity, bool setup){
    /* run ntrig sequences and get the raw data from the DTB */
    
    if(setup) setupDaq(ntrig, ftrigkhz, verbosity);
    
	if(ntrig>0){
	  fApi->daqTrigger(ntrig, fPeriod);
	}

    getBuffer( buf );
    if(buf.size()==0){
        if (verbosity>0){ out << "no data !" << endl;}
        if (setup) restoreDaq(verbosity);
        return 1; 
    }
    
    if(setup) restoreDaq(verbosity);
    return 0;
}



int CmdProc::runDaqRaw(int ntrig, int ftrigkhz, int verbosity){
    /* run ntrig sequences but don't get the data */
    
    int length=0;
    if((ntrig==1) || (ftrigkhz==0)){
        length=0;
    }else{
        length = 40000 / ftrigkhz;
    }
    
    pg_sequence( fSeq, length ); // set up the pattern generator
    fApi->daqTriggerSource("pg_dir");

    bool stat = fApi->daqStart(fBufsize, fPixelConfigNeeded);
    if (! stat ){
        if(verbosity>0){ out << "something wrong with daqstart !!!" << endl;}
        return 2;
    }
    
    if(verbose) out << "runDaq  f=" << dec <<  ftrigkhz  << "   length=" << length << "  fPeriod=" << fPeriod << endl;
    int leff = 40* int(length / 40);  // emulate testboards cDelay
    if((leff<length)&&(ntrig>1)){
        out << "period will be truncated to " << dec<< leff << " BC  = " << int(40000/leff) << " kHz !!" << endl;
    }
    if (length>0){
        fApi->daqTrigger(ntrig, length);
    }else{
        // take the default (maximum)
        fApi->daqTrigger(ntrig, fPeriod);
    }      
    
    fApi->daqStop(false);
    fPixelConfigNeeded = false;
    pg_restore(); // undo any changes
    
    return 0;
}

int CmdProc::runDaqPg(int ntrig, int ftrigkHz, bool randomTriggers,  int verbosity){
    vector<uint16_t> buf;
    vector<DRecord> data;
    runDaqPg(buf, data, ntrig, ftrigkHz, randomTriggers, verbosity);
    return 0;
}



int CmdProc::runDaqPg(vector<uint16_t> & buf, vector<DRecord> & data, 
    int ntrig, int ftrigkHz, bool randomTriggers, int verbosity){
    /* run with some kind of triggers, make it work for pg and random eventually */
    
        
    if( ftrigkHz==0 ){
        out << "invalid settings \n";
        return 0;
    }
   
    size_t nroc=16;
    size_t nchannel=0;
    
    long long int ntrigTotal=0;
    long long int ntrailerTotal=0;
    long long int nerrTotal=0;
    long long int nhitTotal=0;
    float ttotal_s=0;
    
    int nloop=0;
    resetDaqStatus();
    int bufsize=DTB_SOURCE_BUFFER_SIZE/2;
    
    bool fill=true;
    vector< vector< vector< long long int > > > hitmap;
    for(unsigned int roc=0; roc<nroc; roc++){
        vector<vector<long long int> > rocmap(52, vector<long long int>(80)); 
        hitmap.push_back( rocmap );
    }
    vector< long long int > hits(1001);
    vector< vector< long long int > > token(8, vector< long long int >(65) );
    
    uint16_t daqflags = FLAG_DUMP_FLAWED_EVENTS | FLAG_ENABLE_XORSUM_LOGGING;  // has no effect if we read raw data
     
     
    // set up the pattern generator
    int length=0;
    if(randomTriggers){
        // any specific setup needed?
    }else{
        if((ntrig==1) || (ftrigkHz==0)){
            length=0;
        }else{
            length = 40000 / ftrigkHz;
        }
        pg_sequence( fSeq, length ); // set up the pattern generator
        fApi->daqTriggerSource("pg_dir");
    }


    
    // start the daq loop
    while( (ntrig==0) || (ntrigTotal < (ntrig -sqrt(ntrig))) ){
        
        float trun_ms = 1000.;
        
        if (ntrig>0){
            trun_ms = min( double(ntrig)/double(ftrigkHz), min(float(bufsize)/(ftrigkHz*100.), 1000.));
        }
        if(trun_ms < 1.) break;
        drainBuffer();
        if(randomTriggers){
            fApi->daqTriggerSource("random_dir", 0);
            //cout << "configure trigger source \n"; 
            pxar::mDelay(10);
            //cout << "starting daq \n";
            bool startstat = fApi->daqStart(daqflags, bufsize, fPixelConfigNeeded);
            fPixelConfigNeeded=false;
            if(!startstat){ cout << "daq not started !!\n"; }
            fPixelConfigNeeded=false;
            fApi->daqTriggerSource("random_dir", 1000.*ftrigkHz);
        }else{
            //cout << "calling api->daqTriggerLoop " << length << endl;
            fApi->daqStart(bufsize, fPixelConfigNeeded); // otherwise daqTriggerLoop does nothing (status())
            fPixelConfigNeeded=false;
            fApi->daqTriggerLoop( length );
            fPgRunning = true;
       } 
        
        float t=0;
        while( (t<trun_ms) && fApi->daqStatus()){
            pxar::mDelay( 5 ); 
            t+=5;
        }
        if(randomTriggers){
            fApi->daqTriggerSource("random_dir", 0); // stop triggers
            fApi->daqTriggerLoopHalt(); // just need the flush
        }else{
            fApi->daqTriggerLoopHalt(); 
            fPgRunning = false;
        }

        //cout << " stopped triggers and flushed \n";
        pxar::mDelay(10);
        //cout << " stopping daq\n";
        bool stopstat = fApi->daqStop(false);
        if ( !stopstat ){ cout << "daq not stopped !!!\n";}
        getBuffer( buf );
        ttotal_s += t*0.001;
                

        if(buf.size()==0){
            if (verbosity>0){ out << "no data !" << endl;}
            fApi->daqTriggerSource("pg_direct");
            restoreDaq(verbosity);
            return 1; 
        }
        
        nloop++;
        
        data.clear();
        int stat = getData(buf, data, 0, nroc, false); // verbosity, nroc, resetDaqStatus
        
        // dump data if an error occurred 
        if(stat>0){
            if (verbosity>0){
                out<< dec  << stat << " errors!  header #=";
                for(unsigned int n=0; n<fHeadersWithErrors.size(); n++){
                    out << fHeadersWithErrors[n] << " ";
                }
                out << "\n";
            }   
            nerrTotal+=stat;
            
            char filename[200]="";
            snprintf(filename, 50, "randump-%d.txt", nloop);
            ofstream fout(filename);
            dumpBuffer( buf, fout);
            fout.close();
        }
        
        ntrigTotal += fNumberOfEvents;
        if(verbosity>0){
            vector<int> hits = countHits( data, nroc );
            for(size_t i=0; i<nroc; i++){ cout << dec << setfill(' ') <<setw(10) << hits[i];} cout<< endl;
            int nhit=0;
            for(size_t i=0; i<nroc; i++){ nhit += hits[i];}
            nhitTotal += nhit;
            out << dec << setw(4) << (int) t << " ms";
            out << dec << setw(8) << fNumberOfEvents<< " events,  ";
            out << setw(9) <<  nhit << " hits";
            out << setw(5) <<  stat << " errors,   "  ;
            out << "    (" << setw(4) << int(ntrigTotal / 1000000) << " M";
            out << " "  << setw(5) << int(nhitTotal/1000000) << " M  ";
            out << setw(4) << nerrTotal  << " )  \n";
            flush(out);
        }
        
        if(fill){
            uint8_t col, row, ph;
            int nhit=0; // hits per event
            for(unsigned int i=0; i<data.size(); i++){
                
                if ( (data[i].type==DRecord::TBM_HEADER) && (data[i].channel>nchannel) ){
                    nchannel = data[i].channel;
                }
                
                if ( (data[i].type==DRecord::HIT) && (data[i].id<nroc) ){
                    nhit++;
                    int err = pixDecodeRaw(data[i].data, col, row, ph);
                    if(err==0){
                        if( (data[i].id<nroc)&&(col<52)&&(row<80) ){
                            hitmap[data[i].id][col][row]++;
                        }
                    }else{
                        fDaqErrorCount[data[i].channel]+=1;
                    }
                }
                
                if( data[i].type==DRecord::TRAILER ){
                    ntrailerTotal+=1;
                    uint8_t stack = data[i].data & 0x3f;
                    if (stack<32){
                        token[data[i].channel][stack]++;
                    }
                }
                
                if( data[i].type==DRecord::TRAILER && data[i].channel==nchannel){
                    hits[min(1000,nhit)]++;
                    nhit=0;
                }
                
            }

        }
        
        if (stopped()) {out << "stopped by user" << endl; break;}
    }
  
    fApi->daqTriggerSource("pg_direct");
    pg_restore(); // undo any changes

    if(verbosity>0){
        out << dec << ntrigTotal << " events in " << ttotal_s << " seconds, " << "   errors " << nerrTotal << endl;
        if(ntrigTotal >0){
            out << "number of hits per event " << dec << float(nhitTotal)/ntrigTotal;
            out << "  ( " <<  float(nhitTotal)/ntrigTotal/(16*81*0.0100*54*0.0150)*40 << " MHz /cm^2 )\n";
        }
        daqStatus();
    }
    
    
    // write-out the run summary if requested (i.e. if fill==true)
    if(fill){
        ofstream h("ranhist.txt");
        
        h << "#events " << dec << ntrigTotal << endl;
      
        for(unsigned int channel=0; channel<8; channel++){
            h << "#token " << dec << token[channel].size() << " " << channel <<  endl;
            for(unsigned int n=0; n<token[channel].size(); n++){ h << token[channel][n] << " ";}
            h << endl;
        }
        
        h << "#hits " << dec << hits.size() << endl;
        for(unsigned int n=0; n<hits.size(); n++){ h << hits[n] << " ";}
        h << endl;
       
        h << "#hitmap " << dec << hitmap.size() << " " << hitmap[0].size() << " " << hitmap[0][0].size() << endl;
        for(unsigned int roc=0; roc<hitmap.size(); roc++){
            h<< "#roc " << roc << endl;
            for(unsigned int col=0; col<hitmap[roc].size(); col++){
                for(unsigned int row=0; row< hitmap[roc][col].size(); row++){
                    h << hitmap[roc][col][row] << " ";
                }
                h << endl;
            }
        }
        h << endl;
        h.close();
    }   
       
    return 0;
}

int CmdProc::runDaqRandom_obsolete(int ntrig, int ftrigkHz, int verbosity){
    vector<uint16_t> buf;
    vector<DRecord> data;
    runDaqRandom_obsolete(buf, data, ntrig, ftrigkHz, verbosity);
    return 0;
}


int CmdProc::runDaqRandom_obsolete(vector<uint16_t> & buf, vector<DRecord> & data, int ntrig, int ftrigkHz, int verbosity){
    /* run with random triggers */
    
    if( ftrigkHz==0 ){
        out << "invalid settings \n";
        return 0;
    }
   
    size_t nroc=16;
    size_t nchannel=0;
    
    long long int ntrigTotal=0;
    long long int ntrailerTotal=0;
    long long int nerrTotal=0;
    long long int nhitTotal=0;
    float ttotal_s=0;
    
    int nloop=0;
    resetDaqStatus();
    int bufsize=DTB_SOURCE_BUFFER_SIZE/2;
    
    bool fill=true;
    vector< vector< vector< long long int > > > hitmap;
    for(unsigned int roc=0; roc<nroc; roc++){
        vector<vector<long long int> > rocmap(52, vector<long long int>(80)); 
        hitmap.push_back( rocmap );
    }
    vector< long long int > hits(1001);
    vector< vector< long long int > > token(8, vector< long long int >(65) );
    
    uint16_t daqflags = FLAG_DUMP_FLAWED_EVENTS | FLAG_ENABLE_XORSUM_LOGGING;  // has no effect if we read raw data
     
    
    while( (ntrig==0) || (ntrigTotal < (ntrig -sqrt(ntrig))) ){
        
        float trun_ms = 1000.;
        
        if (ntrig>0){
            trun_ms = min( double(ntrig)/double(ftrigkHz), min(float(bufsize)/(ftrigkHz*100.), 1000.));
        }
        if(trun_ms < 1.) break;
        drainBuffer();
        fApi->daqTriggerSource("random_dir", 0);
        //cout << "configure trigger source \n"; 
        pxar::mDelay(10);
        //cout << "starting daq \n";
        bool startstat = fApi->daqStart(daqflags, DTB_SOURCE_BUFFER_SIZE, fPixelConfigNeeded);
        if(!startstat){ cout << "daq not started !!\n"; }
        fPixelConfigNeeded=false;
        fApi->daqTriggerSource("random_dir", 1000.*ftrigkHz);
       
        float t=0;
        while( (t<trun_ms) && fApi->daqStatus()){
            pxar::mDelay( 5 ); 
            t+=5;
        }
        fApi->daqTriggerSource("random_dir", 0); // stop triggers
        fApi->daqTriggerLoopHalt(); // just need the flush
        //cout << " stopped triggers and flushed \n";
        pxar::mDelay(10);
        //cout << " stopping daq\n";
        bool stopstat = fApi->daqStop(false);
        if ( !stopstat ){ cout << "daq not stopped !!!\n";}
        getBuffer( buf );
        ttotal_s += t*0.001;
                

        if(buf.size()==0){
            if (verbosity>0){ out << "no data !" << endl;}
            fApi->daqTriggerSource("pg_direct");
            restoreDaq(verbosity);
            return 1; 
        }
        
        nloop++;
        
        data.clear();
        int stat = getData(buf, data, 0, nroc, false); // verbosity, nroc, resetDaqStatus
        
        // dump data if an error occurred 
        if(stat>0){
            if (verbosity>0){
                out<< dec  << stat << " errors!  header #=";
                for(unsigned int n=0; n<fHeadersWithErrors.size(); n++){
                    out << fHeadersWithErrors[n] << " ";
                }
                out << "\n";
            }   
            nerrTotal+=stat;
            
            char filename[200]="";
            snprintf(filename, 50, "randump-%d.txt", nloop);
            ofstream fout(filename);
            dumpBuffer( buf, fout);
            fout.close();
        }
        
        ntrigTotal += fNumberOfEvents;
        if(verbosity>0){
            vector<int> hits = countHits( data, nroc );
            for(size_t i=0; i<nroc; i++){ cout << dec << setfill(' ') <<setw(10) << hits[i];} cout<< endl;
            int nhit=0;
            for(size_t i=0; i<nroc; i++){ nhit += hits[i];}
            nhitTotal += nhit;
            out << dec << setw(4) << (int) t << " ms";
            out << dec << setw(8) << fNumberOfEvents<< " events,  ";
            out << setw(9) <<  nhit << " hits";
            out << setw(5) <<  stat << " errors,   "  ;
            out << "    (" << setw(4) << int(ntrigTotal / 1000000) << " M";
            out << " "  << setw(5) << int(nhitTotal/1000000) << " M  ";
            out << setw(4) << nerrTotal  << " )  \n";
            flush(out);
        }
        
        if(fill){
            uint8_t col, row, ph;
            int nhit=0; // hits per event
            for(unsigned int i=0; i<data.size(); i++){
                
                if ( (data[i].type==DRecord::TBM_HEADER) && (data[i].channel>nchannel) ){
                    nchannel = data[i].channel;
                }
                
                if ( (data[i].type==DRecord::HIT) && (data[i].id<nroc) ){
                    nhit++;
                    int err = pixDecodeRaw(data[i].data, col, row, ph);
                    if(err==0){
                        if( (data[i].id<nroc)&&(col<52)&&(row<80) ){
                            hitmap[data[i].id][col][row]++;
                        }
                    }else{
                        fDaqErrorCount[data[i].channel]+=1;
                    }
                }
                
                if( data[i].type==DRecord::TRAILER ){
                    ntrailerTotal+=1;
                    uint8_t stack = data[i].data & 0x3f;
                    if (stack<32){
                        token[data[i].channel][stack]++;
                    }
                }
                
                if( data[i].type==DRecord::TRAILER && data[i].channel==nchannel){
                    hits[min(1000,nhit)]++;
                    nhit=0;
                }
                
            }

        }
        
        if (stopped()) {out << "stopped by user" << endl; break;}
    }
  
    fApi->daqTriggerSource("pg_direct");
    pg_restore(); // undo any changes

    if(verbosity>0){
        out << dec << ntrigTotal << " events in " << ttotal_s << " seconds, " << "   errors " << nerrTotal << endl;
        if(ntrigTotal >0){
            out << "number of hits per event " << dec << float(nhitTotal)/ntrigTotal;
            out << "  ( " <<  float(nhitTotal)/ntrigTotal/(16*81*0.0100*54*0.0150)*40 << " MHz /cm^2 )\n";
        }
        daqStatus();
    }
    
    
    // write-out the run summary if requested (i.e. if fill==true)
    if(fill){
        ofstream h("ranhist.txt");
        
        h << "#events " << dec << ntrigTotal << endl;
      
        for(unsigned int channel=0; channel<8; channel++){
            h << "#token " << dec << token[channel].size() << " " << channel <<  endl;
            for(unsigned int n=0; n<token[channel].size(); n++){ h << token[channel][n] << " ";}
            h << endl;
        }
        
        h << "#hits " << dec << hits.size() << endl;
        for(unsigned int n=0; n<hits.size(); n++){ h << hits[n] << " ";}
        h << endl;
       
        h << "#hitmap " << dec << hitmap.size() << " " << hitmap[0].size() << " " << hitmap[0][0].size() << endl;
        for(unsigned int roc=0; roc<hitmap.size(); roc++){
            h<< "#roc " << roc << endl;
            for(unsigned int col=0; col<hitmap[roc].size(); col++){
                for(unsigned int row=0; row< hitmap[roc][col].size(); row++){
                    h << hitmap[roc][col][row] << " ";
                }
                h << endl;
            }
        }
        h << endl;
        h.close();
    }   
       
    return 0;
}


int CmdProc::drainBuffer(bool tellme){
    std::vector<uint16_t> buffer;
    try {
        buffer = fApi->daqGetBuffer();
        if ((buffer.size()>0) && tellme) {
            cout << "buffer was not empty!! size= " << buffer.size() << "\n";
        }
    }  catch (pxar::DataNoEvent &){
        return 0;
    }
    return buffer.size();
 }


int CmdProc::maskHotPixels(int ntrig, int ftrigkHz, int multiplier, float percentile){
    /* run ntrig sequences and get the raw data from the DTB */
       
    size_t nroc=16;
    int ntrigTotal=0;
    int nloop=0;
    resetDaqStatus();
    int bufsize=DTB_SOURCE_BUFFER_SIZE;
   
    vector<uint16_t> buf;
    vector<DRecord> data;

    vector< vector< vector< long long int > > > hitmap;
    for(unsigned int roc=0; roc<16; roc++){
        vector<vector<long long int> > rocmap(52, vector<long long int>(80)); 
        hitmap.push_back( rocmap );
    }
    
    while( (ntrig==0) || (ntrigTotal < (ntrig -sqrt(ntrig))) ){
        
        float trun_ms = 1000.;
        
        if (ntrig>0){
            trun_ms = min( double(ntrig)/double(ftrigkHz), min(float(bufsize)/(ftrigkHz*100.), 1000.));
        }
        if(trun_ms < 1.) break;
        fApi->daqTriggerSource("random_dir",0);
        fApi->daqStart(FLAG_DUMP_FLAWED_EVENTS, DTB_SOURCE_BUFFER_SIZE, fPixelConfigNeeded);
        fApi->daqTriggerSource("random_dir",1000.*ftrigkHz);
        float t=0;
        while( (t<trun_ms) && fApi->daqStatus() ){
            pxar::mDelay( 5 );
            t+=5;
        }
        //pxar::mDelay( trun_ms + 5 ); 
        fApi->daqTriggerSource("random_dir",0);
        fApi->daqStop(false);
        getBuffer( buf );
                
        
        nloop++;
        
        data.clear();
        int stat = getData(buf, data, 1, nroc, false); // verbosity, nroc, resetDaqStatus
        if (stat==0){
         
            ntrigTotal += fNumberOfEvents;
            uint8_t col, row, ph;
            for(unsigned int i=0; i<data.size(); i++){
                if ( (data[i].type==0) && (data[i].id<nroc) ){
                    int err = pixDecodeRaw(data[i].data, col, row, ph);
                    if(err==0){
                        hitmap[data[i].id][col][row]++;
                    }
                }
            }
        }   
        if (stopped()) {out << "stopped by user" << endl; break;}
    }
  
    fApi->daqTriggerSource("pg_direct");
    pg_restore(); // undo any changes

    out << "total number of events = " << dec << ntrigTotal  << endl;
     
    vector< long long int >linmap;
    for(unsigned int roc=0; roc<hitmap.size(); roc++){
        for(unsigned int col=0; col<hitmap[roc].size(); col++){
            for(unsigned int row=0; row< hitmap[roc][col].size(); row++){
                linmap.push_back( hitmap[roc][col][row] );
            }
        }
    }
    out << "sorting \n" ; flush(out);
    stable_sort( linmap.begin(), linmap.end() );

    // cut off at a multiple of the percentile
    long long int nmax = multiplier*linmap[ int(percentile*linmap.size()) ];
    
    out << "multiplicity cut at " << nmax  << "\n"; flush(out);
    
    // disable noisy pixels and dump the list for later use
    unsigned int nmask=0;
    for(unsigned int roc=0; roc<hitmap.size(); roc++){
        for(unsigned int col=0; col<hitmap[roc].size(); col++){
            for(unsigned int row=0; row< hitmap[roc][col].size(); row++){
                if(hitmap[roc][col][row]>nmax){
                    fApi->_dut->maskPixel(col, row, true,  roc);
                    out << "roc " << roc << " pixd " << col << " " <<row << "\n";
                    nmask++;
                    flush(out);
                }
            }
        }
    }
    out << nmask << " pixels masked \n";
    return 0;
}

int CmdProc::burst(vector<uint16_t> & buf, int ntrig, int trigsep, int nburst, int caltrig, int verbosity){
    /* run ntrig sequences and get the raw data from the DTB */
    
    // warn user when no data expected
    if( (fApi->_dut->getNTbms()>0) && ((fSeq & 0x02 ) ==0 ) ){
        out << "The current sequence does not contain a trigger!\n";
    }else if( (fApi->_dut->getNTbms()==0) && ((fSeq & 0x01 ) ==0 ) ){
        out <<"The current sequence does not contain a token!\n";
    }
    
    if(verbose) out << dec << "burst   ntrig=" << ntrig << "  separation " << trigsep << "  bursts " << nburst <<"\n";
    vector< pair<string, uint8_t> > pgsetup;
    pgsetup.push_back( make_pair("sync", 10) );
    pgsetup.push_back( make_pair("resr", fTRC) );
    if(caltrig>0){
        if ( (fTCT-trigsep*caltrig)>1){
            pgsetup.push_back( make_pair("cal",  fTCT-trigsep*caltrig ));
        }else{
            out << "warning, illegal timing for calinject requested\n";
            out << "TCT =  " << fTCT << "\n";
            out << "trigsep * caltrig =  " << trigsep << " * " << caltrig << " = " << trigsep*caltrig << "\n";
        }
    }else{
        pgsetup.push_back( make_pair("cal",  fTCT ));
    }
    for(int i=0; i<ntrig; i++){
        pgsetup.push_back( make_pair("trg",  trigsep-1 ));
    }

    if(fApi->_dut->getNTbms()==0){
        pgsetup.push_back( make_pair("token", 0));
    }else{
        pgsetup.push_back( make_pair("none",0) );
    }
    fPeriod = fMaxPeriod;
 
    fApi->setPatternGenerator(pgsetup);
    fApi->daqTriggerSource("pg_dir");

    bool stat = fApi->daqStart(fBufsize, fPixelConfigNeeded);
    if (! stat ){
        if(verbosity>0){ out << "something wrong with daqstart !!!" << endl;}
        return 2;
    }
    fApi->daqTriggerSource("pg_dir");
    fApi->daqTrigger(nburst, fPeriod);
    getBuffer(buf);
    fApi->daqStop(false);
    fPixelConfigNeeded = false;
    
    if(buf.size()==0){
        if(verbosity>0){ out << "no data !" << endl;}
        return 1; 
    }
    pg_restore(); // undo any changes
    return 0;
    
}




int CmdProc::getData(vector<uint16_t> & buf, vector<DRecord > & data, int verbosity, 
    int nroc_expected, bool resetStats){
    // pre-decoding and validity check of raw data in fBuf
    // returns the number of errors
    // record flags:
    //   0 (0x00) = hit         24 bits ("raw")
    //   4 (0x04) = roc header  12 bits
    //  10 (0x0a) = tbm header  16 bits
    //  14 (0x0e) = tbm trailer 16 bits
    //  16  = filler 
    //
    // FIXME, for >1 event this code required fGetBufMethod=1
   
    if(verbosity>100){
        for(unsigned int i=0; i< buf.size(); i++){
            out << hex << setw(4) << setfill('0') << buf[i] << " ";
        }
        out << dec << setfill(' ') << endl;
    }   
    
    unsigned int nerr=0;
    
    data.clear();
    fDeser400err  =  0;
    if (resetStats) resetDaqStatus();
    
    for(size_t i=0; i<17; i++){
        fRocHeaderData[i]=0;
    }
    vector<uint16_t> rocHeaderWord(17);
    vector<uint16_t> rocHeaderBits(17);
    
    fNumberOfEvents = 0;
    fHeaderCount=0;
    fHeadersWithErrors.clear();  // a list of headers with errors
    
    unsigned int i=0;
    if ( fApi->_dut->getNTbms()>0 ) {
        
        uint8_t daqChannel=0; // from tbm header qualifier
        uint8_t roc=0;
        uint8_t tbm=0;
        bool tbmHeaderSeen=false;
        unsigned int nevent=0;
        unsigned int lastEventStart=0;
        
        vector<unsigned int> rocCounter(nDaqChannelMax);

        
        unsigned int nloop=0;
        int fffCounter=0;

        
        while((i<buf.size())&&((nloop++)<buf.size())){
            
            uint16_t flag= ((buf[i]>>12)&0xE);
            
            if (buf[i]&0x1000){
                fDeser400err++;
                fDeser400SymbolErrors[daqChannel]++;
                fDaqErrorCount[daqChannel]++;
            }
            
            if ((buf[i]&0x0fff)==0xfff){
                fffCounter++;
                if (fffCounter>1000){
                    if(verbosity>0) out << "junk data, decoding aborted\n";
                    fDaqErrorCount[daqChannel]++;
                    nerr++;
                    return nerr + fDeser400err;
                 }
            }else{
                fffCounter=0;
            }
                       
            
            if( flag == 0xa ){ // TBM header
            
                tbmHeaderSeen=true;
                fHeaderCount++;

                daqChannel = (buf[i]&0x700)>>8;
                fNTBMHeader[daqChannel]++;
                rocCounter[daqChannel]=0;

                if (buf[i]&0x0800) { // assuming the lower bits are used for the channel nr
                    nerr++; // should be 0
                    fDaqErrorCount[daqChannel]++;
                    if (verbosity>0) out << "illegal deser400 header record \n";
                }

                
                uint8_t h1=buf[i]&0xFF;
                i++;
                if(i>=buf.size()){
                    if(verbosity>0) out << " unexpected end of data\n";
                    nerr++;
                    fHeadersWithErrors.push_back(fHeaderCount);
                    return nerr + fDeser400err;
                }
                
                if (buf[i]&0x1000){
                    fDeser400SymbolErrors[daqChannel]++;
                    fDaqErrorCount[daqChannel]++;
					fDeser400err++;
				}
				                
                if ((buf[i]&0xE000)==0x8000){
                    uint8_t h2 = buf[i]&0xFF;
                    data.push_back( DRecord( daqChannel, 0xa, (h1<<8 | h2), buf[i-1], buf[i]) );
                    i++;
                }else{
                    if(verbosity>0){
                        out << " incomplete header ";
                        out << hex << (int) buf[i-1] << " " << (int) buf[i]  << dec << "\n";
                    }
                    nerr ++;
                    fDaqErrorCount[daqChannel]++;
                }
                continue;
            }// TBM header
            
            
            if( flag == 0x4 ){ // ROC header
            
                if( !tbmHeaderSeen ){
                    nerr++;
                    fDaqErrorCount[daqChannel]++;
                    if(verbosity>0)  out << "ROC header outside TBM header/trailer ["<<(int)i<< "]\n";
                    cout << "ROC header outside TBM header/trailer\n";
                    for(unsigned int ii=lastEventStart; ii<i+1; ii++){
                        cout << hex << setw(4) <<  setfill('0') << buf[ii] << " ";
                    }
                    cout << endl;
                }
                
                if(nroc_expected==0){
                    nerr++;
                    fDaqErrorCount[daqChannel]++;
                    if(verbosity>0) out<< "no rocs expected\n";
                }else{
                    roc ++;
                    unsigned int rocId = rocIdFromReadoutPosition( daqChannel, rocCounter[daqChannel]);
                    if ( rocCounter[daqChannel] < fnRocPerChannel){
                        rocCounter[daqChannel]++;
                    }else{
                        rocId=16;// for unknown
                    }

                    if (rocId>16){
                        cout<< "roc counting error " << dec << (int) daqChannel << setw(4) << dec << (int) rocCounter[daqChannel]<<  setw(4) << dec << (int) fDaqChannelRocIdOffset[daqChannel]<< endl;
                        rocId=0;
                        nerr++;
                        fDaqErrorCount[daqChannel]++;
                    }
                    
                    if( (buf[i]&0x0004)>0 ) {
                        nerr++;
                        fDaqErrorCount[daqChannel]++;
                        if(verbosity>0) out << "zero-bit in roc header not zero\n";
                    }
                    data.push_back( DRecord(daqChannel, 0x4, buf[i]&0x3, buf[i], rocId) );
                    
                    // roc header data
                    rocHeaderWord[rocId] = (rocHeaderWord[rocId] << 1) + (buf[i]&0x0001);
                    rocHeaderBits[rocId]++;

                    if ((buf[i]&0x0002)==2){  // startbit seen
                        if (rocHeaderBits[rocId]==16){
                            fRocHeaderData[rocId] = rocHeaderWord[rocId]&0xffff;
                            uint8_t hrocid= (fRocHeaderData[rocId] & 0xF000) >> 12;
                             
                            if ( !(hrocid == rocId) ){
                                fRocReadBackErrors[daqChannel]++;
                                if( !fIgnoreReadbackErrors ) {
									nerr++;
									fDaqErrorCount[daqChannel]++;
								}
                                if( !fIgnoreReadbackErrors && verbose ){
                                    cout << hex << setw(4) << rocHeaderWord[rocId];
                                    cout  << "    rocid=" << dec <<  setw(2) << (int) rocId;
                                    cout <<  "  <> " << dec <<  setw(2) << (int) hrocid;
                                    cout << "  daqChannel " << (int) daqChannel;
                                    cout << endl;
                                }
                            }else{
                                //cout << " readback ok " << dec<< (int) rocId << "  " << hex<< rocHeaderWord[rocId] << endl;
                            }
                        }
                        rocHeaderWord[rocId]=0;
                        rocHeaderBits[rocId]=0;
                    }else  if(rocHeaderBits[rocId]==16){
                        if( ! fIgnoreReadbackErrors ) {
							nerr++;
						}
                        if( !fIgnoreReadbackErrors && verbose){
                                    cout << "start bit expected ";
                                    cout  << "    rocid=" << dec <<  setw(2) << (int) rocId;
                                    cout << "  daqChannel " << (int) daqChannel;
                                    cout << endl;
                                }

                    }
                  
                                       

                    uint8_t xordata = (buf[i] & 0x0ff0)>>4;
                    if(xordata==0xff){
                        nerr++;
                        fDaqErrorCount[daqChannel]++;
                        if(verbosity>0) out << "Deser400 phase error\n";
                        fDeser400PhaseErrors[daqChannel]++;
                    }else{
                        if (fDeser400XOR[daqChannel]==xordata){
                            // ok
                        }else{
                            if(fDeser400XOR[daqChannel]==0x100){
                                // inital value, ok
                            }else{
                                fDeser400XORChanges[daqChannel]++;
                            }
                            fDeser400XOR[daqChannel]=xordata;
                        }
                        
                        
                    }
                }
                
                i++;
                continue;
            }
            
            
            if (flag == 0x0){  // hit
                if (roc==0) {
                    if(verbosity>0) out << "no hit expected here\n";
                    nerr++;
                    fDaqErrorCount[daqChannel]++;
                }
                int rocId = rocIdFromReadoutPosition( daqChannel, roc-1);
                int d1=buf[i++];
                if(i>=buf.size()){
                    if(verbosity>0) out << " unexpected end of data\n";
                    nerr++;
                    fDaqErrorCount[daqChannel]++;
                    fHeadersWithErrors.push_back(fHeaderCount);
                    return nerr + fDeser400err;
                }
                
                if (buf[i]&0x1000) {
                    fDaqErrorCount[daqChannel]++;
                    fDeser400err++;
                }
                
                flag = (buf[i]>>12)&0xe;
                int d2=buf[i++];
                if (flag == 0x2) {
                    uint32_t raw = ((d1 &0x0fff) << 12) + (d2 & 0x0fff);
                    if( tbmWithDummyHits() && (roc == fnRocPerChannel) && (raw==0xffffff)){
                        data.push_back( DRecord(daqChannel, 15, raw, buf[i-2], buf[i-1], rocId) );
                    }else{
                        data.push_back( DRecord(daqChannel, 0x0, raw, buf[i-2], buf[i-1], rocId) );
                    }
                                        
                }else{
                    if(verbosity>0) {
                        out << " unexpected qualifier in ROC hit:" << (int) flag << " " 
                            << hex << setw(4) << setfill('0')  << buf[i-1] 
                            << dec << setfill(' ') << " at position "<< i-1 << "\n";
                        }
                    nerr++;
                    fDaqErrorCount[daqChannel]++;
                 }
                continue;
            }
            
            if (flag  == 0xe){
                if (!tbmHeaderSeen){
                    cout << "tbm trailer without header [" << (int) i << "]" << endl;
                    for(unsigned int ii=lastEventStart; ii< i+2; ii++){
                        cout << hex << setw(4) <<  setfill('0') << buf[ii] << " ";
                    }
                    cout << endl;
                }
                fNEvent[daqChannel]++;

                tbmHeaderSeen=false;
                // TBM trailer
                if (buf[i]&0x0f00){
                    nerr++;
                    fDaqErrorCount[daqChannel]++;

                     if(verbosity>0) {
                        out << "deser400 error flags: ";
                        if (buf[i]&0x0800) out << "frame ";
                        if (buf[i]&0x0400) out << "code ";
                        if (buf[i]&0x0200) out << "idle ";
                        if (buf[i]&0x0100) out << "missing trailer ";
                        out << " channel " << (int) daqChannel << "\n";
                    }
                    if (buf[i]&0x0800) fDeser400_frame_error[daqChannel]++;
                    if (buf[i]&0x0400) fDeser400_code_error[daqChannel]++;
                    if (buf[i]&0x0200) fDeser400_idle_error[daqChannel]++;
                    if (buf[i]&0x0100) fDeser400_trailer_error[daqChannel]++;

                }
                int t1=buf[i++];
                if(i>=buf.size()){
                    nerr++;
                    fDaqErrorCount[daqChannel]++;

                    if(verbosity>0) out << "unexpected end of data\n";
                    return nerr + fDeser400err;
                }
                
                flag = (buf[i]>>12)&0xe;
                if (flag != 0xc ){
                    if(verbosity>0) out << "unexpected qualifier " << (int) flag <<"in TBM trailer \n";
                    continue;
                }
                /* in fw4.6 (and higher?) this qualifier just repeat the previous entry
                if (buf[i]&0x1000) fDeser400err++;
                if (buf[i]&0x0f00) {
                    nerr++;
                    if(verbosity>0) out << "illegal data in deser400 trailer record \n";
                }
                */
                int t2=buf[i++];
                data.push_back( DRecord(daqChannel, 0xe, (t1&0xFF)<<8 | (t2&(0xff)), t1, t2) );
                tbm++;
                
                if (tbm==fnDaqChannel){
                    tbm=0; // new event, this depends on the getBuffer method
                    nevent++;
                    lastEventStart=i;
                }
                        
                roc=0;
                continue;
            }
            
            if((verbose) || (verbosity>0)){ 
                out << "unexpected qualifier " << hex << (int) flag <<", skipped\n" << dec;
            }
            nerr++;
            fDaqErrorCount[daqChannel]++;
            i++;
            
        } // while i< buf.size()
        
        // debugging
        if ((nloop>=buf.size())&&(buf.size()>1000) ){
                cout << "stuck at i=" << dec << i << endl;
                cout << hex << (int) buf[i-1] << " " << (int) buf[i] << dec << endl;
        }

         fNumberOfEvents = nevent;

    }
    
    else if (fApi->_dut->getNTbms() == 0) {
        // single ROC
        while(i<buf.size() && (i<500) ){
            
            //if ( (buf[i] & 0x0ff8) == 0x07f8 ){
            if ( (buf[i] & 0x8000) == 0x8000 ){
                // roc header
                data.push_back(DRecord(0, 0x4, buf[i]&0x07, buf[i]));
                i++;
            }
            else if( (i+1)<buf.size() ){
                // hit
                 uint32_t raw = ((buf[i] & 0x0fff)  << 12) + (buf[i+1] & 0x0fff);
                 data.push_back(DRecord(0, 0, raw, buf[i], buf[i+1] ) );
                i+=2;
            }else{
                out << "unexpected end of data\n";
                nerr++;
                return nerr;
            }
        }
    }
       
    return nerr + fDeser400err;   
}
    
    
int CmdProc::printData(vector<uint16_t> buf, int level, unsigned int nheader){
    // produce a more or less decoded / annotated printout
   
    if(level >= 0){
        if(level>=0){
            for(unsigned int i=0; i<buf.size(); i++){
                if(fApi->_dut->getNTbms()>0){
                    if ((buf[i]&0xE000)==0xA000){ 
                        out << "\n";
                        vector<unsigned int>::iterator it = find (fHeadersWithErrors.begin(), fHeadersWithErrors.end(), nheader);
                        if (it != fHeadersWithErrors.end()){  
                            out << "*"; 
                        }else{
                            out<< " ";
                        }
                        nheader++;
                    }
                }
                out << " " <<hex << setw(4)<< setfill('0')  << buf[i] << setfill(' ');
            }
        }   
        out << "\n";
    }


    if (level>0){
        //const int maxLines = 200;
        const int maxLines = 1000*200;

        vector< DRecord > data;
        data.clear();
        int stat = getData(buf, data, 0);
        
        uint8_t roc=0;  
        uint8_t tbm=0;
     
        int nevent=0;  // count events
        int lines=0;    // count lines, stop when limit reached
        int nstuff=0;  // count consecutive dummy hits, print only once
        
        for(unsigned int i=0; (i<data.size())&&(lines<maxLines); i++){
            
            char deserStat=' ';
            if((data[i].w1 & 0x1000)>0){
                deserStat='*';
            }
            
            if (data[i].type==15){
                nstuff +=1;
            }else if(nstuff>0){
                out << setw(4) << setfill('0') << hex << data[i-1].w1 
                    << deserStat 
                    << setw(4) << setfill('0') << hex << data[i-1].w2 
                    << setfill(' ') << "  ";
                if (nstuff==1){
                    out << "zero hit\n";
                }else{
                    out << dec << nstuff << "  zero hits\n";
                }
                nstuff=0;
                lines++;
            }
            
            
            if (data[i].type==4){
                
                // ROC header, one 16 bit word
                out << setw(4) << setfill('0') << hex << data[i].w1 << setfill(' ') 
                    << deserStat << "      ";
                out << "ROC header ";
                if (tbm>0){
                    out << dec << setw(2) << (int) rocIdFromReadoutPosition( data[i].channel, roc) ;
                }else{
                    out << dec << setw(2) << (int) roc;
                }
                out << " ,D=" << (data[i].data & 1);
                if ((data[i].data &2 )>0) { out << " S";}
                if((tbm==0)&&(fApi->_dut->getNTbms()>0)) out << ", Warning! TBM header expected first!";
                out << "\n";
                roc ++;
                lines++;
                
            }else if(!(data[i].type==15)){ 
                
                // all others are two 16 bit words
                out << setw(4) << setfill('0') << hex << data[i].w1 
                    << deserStat 
                    << setw(4) << setfill('0') << hex << data[i].w2 
                    << setfill(' ') << "  ";
                lines++;
            
                if (data[i].type==0){ 
                    out << "hit";
                    stat = pixDecodeRaw(data[i].data, level);
                    if(roc==0){
                        out << " Warning! ROC header expected first!";
                    }
                    out << "\n";
                                        
                }else if (data[i].type==0xa){
                    
                    out << "TBM header, ";
                    out << "event counter = " << dec << setfill(' ') << setw(3) << (data[i].data >>8);
                    int h2 = data[i].data&0xFF;
                    int dataId=(h2&0xC0)>>6;
                    out << ", dataID=" << dec << setw(1) <<dataId << " ";
                    if (dataId==2){
                        out << ",mode=" << (int) ((h2&0x30)>>4);
                        if(h2 & 8) { out << ", DisableTrigOut ";}
                        if(h2 & 4) { out << ", DisableTrigIn ";}
                        if(h2 & 2) { out << ", Pause ";}
                        if(h2 & 1) { out << ", DisablePKAM ";}
                    }else{
                        out << ",value=" << hex << setw(2) << setfill('0') << (int)(h2&0x3F) << setfill(' ');
                    }
                    tbm++;
                    out<<"\n";
                    
                }else if (data[i].type==0xe){
                    out << "TBM trailer";
                    uint16_t t= data[i].data;
                    if( t & 0x8000 ) { out << ", NoTokenPass";}
                    if( t & 0x4000 ) { out << ", ResetTBM";}
                    if( t & 0x2000 ) { out << ", ResetROC";}
                    if( t & 0x1000 ) { out << ", SyncError";}
                    if( t & 0x0800 ) { out << ", SyncTrigger";}
                    if( t & 0x0400 ) { out << ", ClrTrgCntr";}
                    if( t & 0x0200 ) { out << ", Cal";}
                    if( t & 0x0100 ) { out << ", StackFullWarn";}
                    if( t & 0x0080 ) { out << ", StackFullnow";}
                    if( t & 0x0040 ) { out << ", PKAMReset";}
                    out <<  ", StackCount=" << (int) (t&0x003F);
                    
                    // also show deserializer status bits
                    if((data[i].w1&0x0f00)>0){
                        out << " [ ";
                        if (data[i].w1&0x0800) out << "frame ";
                        if (data[i].w1&0x0400) out << "code ";
                        if (data[i].w1&0x0200) out << "idle ";
                        if (data[i].w1&0x0100) out << "missing trailer ";
                        out << "]";
                    }

                    roc=0;
                    if(tbm==fnDaqChannel){
                        tbm=0;
                        nevent++;
                    }

                  
                    out<<"\n";
               }
            }
        }
        if(lines==maxLines){
            out << "dump aborted, line limit reached\n";
        }
        
        // count hits (even if the dump was aborted)
        int nhit=0;
        int nRocHeader=0;
        for(unsigned int i=0; i<data.size(); i++){
            if (data[i].type==0) nhit++;
            if (data[i].type==4) nRocHeader++;
        }
        if(nhit>0){
            out << dec << nhit << " hits \n";
        }
        
        if(stat-fDeser400err>0){
            out << dec << stat-fDeser400err << " decoding errors\n";
        }
        if(fDeser400err>0){
            out << dec << fDeser400err <<  " deser400 errors!\n";
        }
        if((nRocHeader>0)&&(fApi->_dut->getNTbms()>0)){
            out << "XOR eye sdata 1       = " << bitset<8>(fDeser400XOR[0]|fDeser400XOR[1]) << endl;
            //if(tbm09) out << "XOR eye sdata 2       = " << bitset<8>(fDeser400XOR[2]||fDeser400XOR[3]) << endl;
        }
    }

    return 0;   
}

int CmdProc::dumpBuffer(vector<uint16_t> buf, ofstream & fout, int level){
    // dump data to a file

    unsigned int ntbma,ntbm8,nroc,ntbme,ntbmc;
    bool ntp=false;
    //bool autoreset=false;
    bool deser400error=false;
    ntbma=ntbm8=nroc=ntbme=ntbmc=0;
    if(level>=0){
        stringstream s;
        for(unsigned int i=0; i<buf.size()+1; i++){
            if ( (i==buf.size()) || ((buf[i]&0xE000)==0xA000) ){ 
                if((ntbma==1)&&(ntbm8==1)&&(nroc==fnRocPerChannel)&&(ntbme==1)&&(ntbmc==1)&&(!deser400error)){
                    fout << " " << s.str() << "\n";
                }else if(ntbma==0){
                    fout << s.str() << "\n";
                }else if( (nroc==0) && ntp ){
                    fout << "N" << s.str() << "\n";
                }else{
                    fout << "X" << s.str() << "\n";
                }
                s.str("");
                ntbma=1;
                ntbm8=nroc=ntbme=ntbmc=0;
            }
            if(i<buf.size()){
                if((buf[i]&0xE000)==0x8000) ntbm8+=1;
                if((buf[i]&0xE000)==0x4000) nroc+=1;
                if((buf[i]&0xE000)==0xe000) ntbme+=1;
                if((buf[i]&0xE000)==0xc000) ntbmc+=1;
                if((buf[i]&0xE000)==0xe000){
                    ntp = (buf[i]&0x0080) >0;
                    //autoreset = (buf[i]&0x0008)>0;
                    deser400error = (buf[i]&0x0F00)>0;
                }
                s << " " <<hex << setw(4)<< setfill('0')  << buf[i] << setfill(' ');
            }
        }
    }   
    fout << "\n";
    return 0;
}


int CmdProc::daqStatus(){
    /* information about the most recent daq run */
    out << "Ch      events  phase-changes  errors  symbol phase readbck  frame     code    idle trailer    XOR\n" ;
    for(unsigned i=0; i<fnDaqChannel; i++){
        out << setw(2) << dec << i;
        out  << setw(12) << dec << fNTBMHeader[i];
        out  << setw(10) << dec << fDeser400XORChanges[i];
        out  << setw(10) << dec << fDaqErrorCount[i];
        out  << setw(8) << dec << fDeser400SymbolErrors[i];
        out  << setw(8) << dec << fDeser400PhaseErrors[i];
        out  << setw(8) << dec << fRocReadBackErrors[i];
        out  << setw(8) << dec << fDeser400_frame_error[i];
        out  << setw(8) << dec << fDeser400_code_error[i];
        out  << setw(8) << dec << fDeser400_idle_error[i];
        out  << setw(8) << dec << fDeser400_trailer_error[i];

        out <<"    ";
        for(unsigned int b=0; b<8; b++){
            out << (int) ((fDeser400XOR[i]>>(7-b))&1 );
        }
        out << "\n";
    }     
    return 0;
}


int CmdProc::resetDaqStatus(){
    for(size_t i=0; i<nDaqChannelMax; i++){
        fDeser400XOR[i]=0x100;
        fDeser400SymbolErrors[i]=0;
        fDeser400PhaseErrors[i]=0;
        fDeser400XORChanges[i]=0;
        fNTBMHeader[i]=0;
        fNEvent[i]=0;
        fRocReadBackErrors[i]=0;
        fDeser400_frame_error[i]=0;
        fDeser400_code_error[i]=0;
        fDeser400_idle_error[i]=0;
        fDeser400_trailer_error[i]=0;
        fDaqErrorCount[i]=0;
    }
    return 0;
}

int CmdProc::sequence(int seq){
    // update the sequence, but only re-program the DTB if actually running
    fSeq = seq;
    if( fPgRunning ) pg_sequence(seq);
    return 0;
}
int CmdProc::pg_loop(int value){
    uint16_t delay = pg_sequence( fSeq );
    fApi->daqStart(fBufsize, false); // otherwise daqTriggerLoop does nothing (status())
    if(value==0){
        if(verbose) cout << "calling api->daqTriggerLoop " << fBufsize << endl;
        uint16_t retval = fApi->daqTriggerLoop( delay );
        if(verbose) cout << "return value " << retval << endl;
    }else{
        if(verbose) cout << "calling api->daqTriggerLoop " << value << endl;
        uint16_t retval = fApi->daqTriggerLoop( value );
        if(verbose) cout << "return value " << retval << endl;
    }        
    fPgRunning = true;
    return 0;
}

int CmdProc::pg_stop(){
    fApi->daqTriggerLoopHalt();
    fApi->daqStop(false);
    fPgRunning = false;
    pg_restore();
    return 0;
}



int CmdProc::pg_sequence(int seq, int length){
    // configure the DTB pattern generator for a simple sequence
    // as other tests don't seem to take care of the pg configuration,
    // this must be undone afterwards (using pg_restore)
    // if length is 0 (default), the full buffersize is used for fPeriod
    // the calculated sequence length is returned as a return value
    vector< pair<string, uint8_t> > pgsetup;
    uint16_t delay = 15 + 7 + 8*3 + 7 + 8*6; // module minimal readout time, allow 8 hits
    if (seq & 0x20 ) { pgsetup.push_back( make_pair("resettbm", 10) ); delay+=11;}
    if (seq & 0x10 ) { pgsetup.push_back( make_pair("sync", 10) ); delay+=11;}
    if (seq & 0x40 ) { pgsetup.push_back( make_pair("resr", fTRC) ); delay+=fTRC+1; }
    if (seq & 0x08 ) { pgsetup.push_back( make_pair("resr", fTRC) ); delay+=fTRC+1; }
    if (seq & 0x04 ) { pgsetup.push_back( make_pair("cal",  fTCT )); delay+=fTCT+1; }
    if (seq & 0x02 ) { pgsetup.push_back( make_pair("trg",  fTTK ));  delay+=fTTK+1;}
    if (seq & 0x01 ) { pgsetup.push_back( make_pair("token", 1)); delay+=1; }
    pgsetup.push_back(make_pair("none", 0));

    
    if (length==0){
        fPeriod = fMaxPeriod;
    }else{
        fPeriod = length;
    }
 
    fApi->setPatternGenerator(pgsetup);
    return delay;
}


int CmdProc::pg_restore(){
    // restore the default pattern generator so that it works for other tests
    std::vector<std::pair<std::string,uint8_t> > pg_setup
        =  fPixSetup->getConfigParameters()->getTbPgSettings();
    if(verbose){
        for(std::vector<std::pair<std::string,uint8_t> >::iterator it=pg_setup.begin(); it!=pg_setup.end(); it++){
            cout << it->first << "  : " << (int) it->second << endl;
        }
    }
	fApi->setPatternGenerator(pg_setup);
    return 0;
}


/**************** call-backs for script processing ***********************/

int CmdProc::tb(Keyword kw){
    /* handle testboard commands 
     * return -1 for unrecognized commands
     * return  0 for success
     * return >0 for errors
     */

    int step, pattern, delay, value;
    string s, comment, filename;
    if( kw.match("ia")    ){  out <<  "ia=" << setw(6)<< setprecision(3)<<fApi->getTBia()<<"\n"; return 0;}
    if( kw.match("id")    ){  out <<  "id=" << setw(6)<< setprecision(3)<<fApi->getTBid()<<"\n"; return 0;}
    if( kw.match("va")    ){  out <<  "va=" << setw(6)<< setprecision(3)<<fApi->getTBva()<<"\n"; return 0;}
    if( kw.match("vd")    ){  out <<  "vd=" << setw(6)<< setprecision(3)<<fApi->getTBvd()<<"\n"; return 0;}
    if( kw.match("set","vd", value)){ return setTestboardPower("vd", value);  }
    if( kw.match("set","va", value)){ return setTestboardPower("va", value);  }
    if( kw.match("getia") ){  out <<  "ia=" << setw(6)<< setprecision(3)<<fApi->getTBia() <<"mA\n"; return 0;}
    if( kw.match("getid") ){  out <<  "id=" << setw(6)<< setprecision(3)<<fApi->getTBid() <<"mA\n"; return 0;}
    if( kw.match("hvon")  ){ fApi->HVon(); return 0; }
    if( kw.match("hvoff") ){ fApi->HVoff(); return 0; }
    if( kw.match("pon")   ){ fApi->Pon(); return 0; }
    if( kw.match("poff")  ){ fApi->Poff(); return 0; }
    if( kw.match("d1") || kw.match("d2") ){
        for(unsigned int i=0; i<fD_names.size(); i++){ out << " " <<fD_names[i]; }
		return 0;
	};
	if( kw.match("a1") || kw.match("a2") ){
        for(unsigned int i=0; i<fA_names.size(); i++){ out << " " <<fA_names[i]; }
		return 0;
	};
    if( kw.match("d1", s, fD_names, out ) ){ fApi->SignalProbe("D1",s); return 0;}
    if( kw.match("d2", s, fD_names, out ) ){ fApi->SignalProbe("D2",s); return 0;}
    int channel;
    if( kw.match("d1", s, fD_names, channel, out) ){ fApi->SignalProbe("D1",s,channel); return 0;}
    if( kw.match("d2", s, fD_names, channel, out) ){ fApi->SignalProbe("D2",s,channel); return 0;}

    //if( kw.match("d2", "deser_idle_error", channel) ){ fApi->SignalProbe("D2","deser_idle_error",channel); return 0;}
    if( kw.match("a1", s, fA_names, out ) ){ fApi->SignalProbe("A1",s); return 0;}
    if( kw.match("a2", s, fA_names, out ) ){ fApi->SignalProbe("A2",s); return 0;}
    if( kw.match("adc", s, fA_names, out ) ){ fApi->SignalProbe("adc",s); return 0;}  
    if( kw.match("clock","external") ){ fApi->setExternalClock(true); return 0;}
    if( kw.match("clock","internal") ){ fApi->setExternalClock(false); return 0;}    
    if( kw.match("seq", pattern)){ sequence(pattern); return 0;}
    if( kw.match("seq","t")){ sequence( 2 ); return 0; }
    if( kw.match("seq","c")){ sequence( 4 ); return 0; }
    if( kw.match("seq","r")){ sequence( 8 ); return 0; }
    if( kw.match("seq","ct")){ sequence( 6 ); return 0; }
    if( kw.match("seq","rt")){ sequence( 10 ); return 0; }
    if( kw.match("seq","rc")){ sequence( 12 ); return 0; }
    if( kw.match("seq","rct")){ sequence( 14 ); return 0; }
    if( kw.match("seq","ctt")){ sequence( 7 ); return 0; }
    if( kw.match("seq","rctt")){ sequence( 15 ); return 0; }
    if( kw.match("pg","restore")){ return pg_restore();}
    if( kw.match("loop") ){ return pg_loop();}
    if( kw.match("loop", value) ){ return pg_loop(value);}
    if( kw.match("stop") ){ return pg_stop();}
    if( kw.match("pg","loop") ){ return pg_loop();}
    if( kw.match("pg","stop") ){ return pg_stop();}
    //if( kw.match("res") ){ int s0=fSeq; sequence( 8 ); pg_single(); sequence(s0);}
    if( kw.match("sighi",s)){ fApi->setSignalMode(s,2); return 0;}
    if( kw.match("siglo",s)){ fApi->setSignalMode(s,1); return 0;}
    if( kw.match("signorm",s)){ fApi->setSignalMode(s,0); return 0;}
    if( kw.match("level",value)){ setTestboardDelay("level", value); return 0;}
    if( kw.match("clk",value)){ setTestboardDelay("clk", value); return 0;}
    if( kw.match("sda",value)){ setTestboardDelay("sda", value); return 0;}
    if( kw.match("sda")){ out << dec << (int) getTestboardDelay("sda"); return 0;}
    if( kw.match("ctr",value)){ setTestboardDelay("ctr", value); return 0;}
    if( kw.match("tbdly",value)){ 
            setTestboardDelay("clk",value); 
            setTestboardDelay("ctr",value); 
            setTestboardDelay("sda",value+15); 
            setTestboardDelay("tin",value); 
            return 0;
        }
    if( kw.match("trimdelay", value)){ setTestboardDelay("trimdelay",value); return 0; }
    if( kw.match("triggerdelay", value)){ setTestboardDelay("triggerdelay",value); return 0; }
    if( kw.match("programdut")){ fApi->programDUT(); return 0;}
    //if( kw.match("aux",value)){ fApi->aux(value); return 0;}
    if( kw.match("adctest", s, fA_names, out ) ){ adctest(s); return 0;} 
    if( kw.match("adctest") ){ 
        adctest("clk"); 
        adctest("ctr"); 
        adctest("sda"); 
        adctest("rda"); 
        adctest("sdata1"); 
        adctest("sdata2"); 
        return 0;
    } 
    if( kw.match("tbmread") || kw.match("readback","tbm") ){return tbmreadback();}
    if( kw.match("read","stack")  ){ printStack(tbmreadstack(), out); flush(out); return 0;}
    if( kw.match("monitor","stack",value) ){return monitorStack(value);}
    int cycles=0, run=0;
    if( kw.match("seu",value) ){return monitorStack(value, run);}
    if( kw.match("seu",value, cycles) ){return monitorStack(value, cycles);}
    if( kw.match("seu",value, cycles, run) ){return monitorStack(value, cycles, run);}
    if( kw.match("readback")) { return readRocs();}
    if( kw.match("readback", value)){ return readRocs(value); }
    if( kw.match("readback", "vd")  ) { return readRocs(8, 0.016,"V");  }
    if( kw.match("readback", "va")  ) { return readRocs(9, 0.016,"V");  }
    if( kw.match("readback", "vana")) { return readRocs(10, 0.008,"V"); }
    if( kw.match("readback", "vbg") ) { return readRocs(11, 0.008,"V"); }
    if( kw.match("readback", "iana")) { return readRocs(12, 0.24,"mA"); }
    if( kw.match("readback", "ia")  ) { return readRocs(12, 0.24,"mA"); }
    if( kw.match("read", "vd")  ) { return readRocsAnalog(8, 0.016,"V");  }
    if( kw.match("read", "va")  ) { return readRocsAnalog(9, 0.016,"V");  }
    if( kw.match("read", "vana")) { return readRocsAnalog(10, 0.008,"V"); }
    if( kw.match("read", "iana")) { return readRocsAnalog(12, 0.24,"mA"); }
    if( kw.match("read", "ia")  ) { return readRocsAnalog(12, 0.24,"mA"); }
    if( kw.match("scan","tct")){ tctscan(); return 0;}
    if( kw.match("tct", value)){ fTCT=value; if (fSeq>0){sequence(fSeq);} return 0;}
    if( kw.match("ttk", value)){ fTTK=value; if (fSeq>0){sequence(fSeq);} return 0;}
    if( kw.match("trc", value)){ fTRC=value; if (fSeq>0){sequence(fSeq);} return 0;}
    if( kw.match("getbufmethod",value)){ fGetBufMethod=value; return 0;}
    if( kw.match("config",value)){ fPixelConfigNeeded=(value>0); return 0;}
        
    if( kw.match("dread") ){
      fApi->daqTriggerSource("pg_dir");
        fApi->daqStart(fBufsize, fPixelConfigNeeded);
        fApi->daqTrigger(1, fPeriod);
        std::vector<pxar::Event> buf;
	try { buf = fApi->daqGetEventBuffer(); }
	catch(pxar::DataNoEvent &) {}
        fApi->daqStop(false);
        for(unsigned int i=0; i<buf.size(); i++){
            out  << buf[i];
        }
        out << " ["<<buf.size()<<"]\n";
        fPixelConfigNeeded = false;
        return 0;
    }
    
    if( kw.match("tbmtest","rda")){
		uint8_t value;
		for(int core=0; core<2; core++){
			int stat = tbmget("base0", 1<<core, value);
			uint8_t addr= (core==0) ? 0xe1 : 0xf1;
			string name= (core==0) ? "A" : "B";
			if(stat==0){
				uint8_t testvalue= (~value) | 0x02; // don't shut down the clock
				tbmset("base0", 1 << core, testvalue);

				int readvalue = tbmread(addr, 31); // TODO: needs a fix, expand similar to tbmreadback()
				tbmset("base0",1 << core, value);
				if( readvalue == (int) testvalue){
					out << "core " << name << " write/read ok\n";
				}else{
					out << "core "<<name <<" write/read failed\n";
				}
			}else{
				out << "Error retrieving base0 from api\n";
			}
		}
		return 0;
	}
	
    if( kw.match("tbmtest","trigger") ){
		// inject a tbm generated trigger and read out the data
		// inject the trigger, both cores
		pg_sequence( 0 ); // no trigger
		fApi->daqStart(fBufsize, fPixelConfigNeeded);
		fPixelConfigNeeded = false;
		out <<"TBM trigger test \n";
		out << "\ncore A:";
		tbmset("base4",TBMA, (1<<6)+1);
		//tbmset("base4",0,      1);
		fApi->daqTrigger(1, fPeriod);
		tbmset("base4",TBMA, (1<<6));// clear token, just in case
		getBuffer(fBuf);
		printData( fBuf, 0 );

		out << "\ncore B:";
		tbmset("base4",TBMB,  (1<<6)+    1);
		fApi->daqTrigger(1, fPeriod);
		tbmset("base4",TBMB, (1<<6));// clear token, just in case

		getBuffer(fBuf);
		printData( fBuf, 0 );

		out << "\nboth cores:";  
		tbmset("base4",ALLTBMS, (1<<6)+     1); // inject trigger
 		fApi->daqTrigger(1, fPeriod);
		tbmset("base4",ALLTBMS, (1<<6));// clear token, just in case
		getBuffer(fBuf);
		printData( fBuf, 0 );

		fApi->daqStop(false);
        return 0;
    }
	
    int bufmethod=0; // use api->getBuffer (better when there is no trailer)
    if( kw.match("raw") || kw.match("raw",bufmethod) ){
        int bufMethodRestore = fGetBufMethod;
        fGetBufMethod = bufmethod;
        int stat = runDaqRaw( fBuf, 1,0, 0 );
        if(stat==0){
            printData( fBuf, 0 );
        }else{
            out << " error getting data ("<<dec<<(int)stat<<")\n";
            printData(fBuf, 0);
        }
        fGetBufMethod = bufMethodRestore;
        return 0;
    }
    
    
    if( kw.match("adc")){
        int stat = runDaqRaw( fBuf, 1, 0, 1 );
        if(stat==0){
            printData( fBuf, 1 );
        }else{
            out << " error getting data\n";
            printData(fBuf, 1);
        }
        return 0;
    }
    
   
        
    int ntrig;
    int ftrigkhz=1;
    if( kw.match("errors", ntrig) || kw.match("errors", ntrig, ftrigkhz) ){
        int stat=countErrors(ntrig, 1, 16);
        if(stat<0){
            out << "no data\n";
        }else{
            out << dec<< (int) stat << "\n";
        }
        return 0;
    }

    int nloop=1;
    ntrig=100; ftrigkhz=10;
    if( kw.match("good") || kw.match("good", nloop )|| kw.match("good", nloop, ntrig )  || kw.match("good", nloop, ntrig, ftrigkhz) ){
        int ngood = countGood(nloop, ntrig, ftrigkhz, 16);
        
        out << dec<< (int) ngood  << " x " << ntrig << " error free  (out of "<< (int) nloop << ")";
        if ( (ngood==nloop) ) {
            out<<" perfect\n";
        }else{
            out<<" there were errors!";
        }
        return 0;
    }

    if (kw.match("daqstatus")){ daqStatus(); return 0;}

    if( kw.match("daq", ntrig, ftrigkhz) ){
        
        if(fPrerun>0) runDaqRaw(ntrig, ftrigkhz);
        runDaqRaw(fBuf, ntrig, ftrigkhz);
        
        vector<DRecord > data;
        int stat = getData(fBuf, data, 0);
        
        int n=0;
        
        if(fApi->_dut->getNTbms()==0){
            // single ROC
            for(unsigned int i=0; i<fBuf.size(); i++){
                
                //if ((fBuf[i]&0x0ff8)==0x07f8) {
                if((fBuf[i]&0x8000)==0x8000){
                   if(i>0) out << "\n";
                    out << dec << setfill(' ') << setw(4) << ++n << ": ";
                }
                out << setw(4) << setfill('0') << hex << fBuf[i] << " ";
            }
            out << setfill(' ') << endl;            
            out<< dec  << stat << " errors\n";
        }else{
            for(unsigned int i=0; i<fBuf.size(); i++){
                
                if((fBuf[i]&0xe000)==0xa000) {
                    if(i>0) out << "\n";
                    out << dec << setfill(' ') << setw(4) << ++n << ": ";
                }
                out << setw(4) << setfill('0') << hex << fBuf[i] << " ";
            }
            out << setfill(' ') << endl;
            if(stat==0){
                out << "no errors\n";
            }else{
                out<< dec  << stat << " errors!  header #=";
               for(unsigned int n=0; n<fHeadersWithErrors.size(); n++){
                    out << fHeadersWithErrors[n] << " ";
                }
                out << "\n";
            }
        }
        return 0;
    }

    ntrig=10000;
    int nhittarget=5000;
    int dacstart=50;
    int dacstop = 140;
    if(kw.match("noise" ) || kw.match("noise", ntrig, nhittarget )
     || kw.match("noise", ntrig, nhittarget, dacstart ) 
     || kw.match("noise", ntrig, nhittarget, dacstart, dacstop )  ){
        // do unmask first !
        size_t nroc=16;
        vector<uint8_t> vthr(nroc);

        vector<DRecord> data;
        for( uint8_t v=dacstart; v<dacstop; v++){
            
            for(size_t i=0; i<nroc; i++){
                if (vthr[i]==0){
                   fApi->setDAC("vthrcomp", v, i ); 
                }else{
                   fApi->setDAC("vthrcomp", vthr[i], i ); 
                }
            }
            tbmset("base4",ALLTBMS, (1<<2));// reset rocs
            
            runDaqPg(fBuf, data, ntrig, 20,  true, 0);
            vector<int> hits=countHits( data, nroc );
            
            bool alldone=true;
            out << setw(3) << dec << (int) v;
            for(unsigned int i=0; i<nroc; i++){
                out << setw(5) << hits[i];
                if ( vthr[i]==0) {
                    if (hits[i]>nhittarget){
                        vthr[i]=v;
                    }else{
                        alldone=false;
                    }
                }
            }
            out << "\n";
            flush(out);
            if(alldone || stopped()) break;
        }
        
        out <<"    ";
        for(unsigned int i=0; i<nroc; i++){
            out << setw(6) << (int) vthr[i];
        }
        out << "\n";
        
        out << "roc * vthrcomp " << setw(0) << (int) vthr[0];
        for(unsigned int i=1; i<nroc; i++){
            out << "," <<  (int) vthr[i];
        }
        out << "\n";

        flush(out);
        
        return 0;
    }

    nhittarget=0;
    ntrig = 10000;
    if( kw.match("calm") ||  kw.match("calm" ,ntrig) || kw.match("calm" ,ntrig, nhittarget)) {
        // do unmask first !
        size_t nroc=16;
        vector<uint8_t> vthr0(nroc);
        vector<uint8_t> vthr(nroc);
        for(size_t i=0; i<nroc; i++){
            vthr[i] = fApi->_dut->getDAC(i, "vthrcomp"); 
            vthr0[i] = fApi->_dut->getDAC(i, "vthrcomp"); 
        }
        out <<"vthrc" << dec;
        for(unsigned int i=0; i<nroc; i++){
            out << setw(7) << (int) vthr[i];
        }
        out << "\n";
        
        vector<DRecord> data;
        
        while( true ){
            
            tbmset("base4",ALLTBMS, (1<<2));// reset rocs
            
            runDaqPg(fBuf, data, ntrig, 20,  true, 0);
            vector<int> hits=countHits( data, nroc );
            
            bool alldone=true;
            out <<"      ";
            unsigned int rocx=0;
            for(unsigned int i=1; i<nroc; i++){
                if(hits[i]>hits[rocx]) { rocx =i ;}
            }
            for(unsigned int i=0; i<nroc; i++){
                out << dec << setw(6) << hits[i];
                if (((i==rocx)&&(hits[i]>nhittarget))|| (hits[i] > 100*max(nhittarget,1)) ){
                    out << "*";
                    if(vthr[i]>0){
                        vthr[i]--;
                        fApi->setDAC("vthrcomp", vthr[i], i);
                        alldone=false;
                    }
                }else{
                    out <<" ";
                }
            }
            out << "  id=" << fixed << setw(6)<< setprecision(3)<<fApi->getTBid()<<resetiosflags(ios::scientific )<<"\n"; 
            flush(out);
            if(alldone || stopped()) break;
        }
        
        out <<"vthrc";
        for(unsigned int i=0; i<nroc; i++){
            out << setw(6) << (int) vthr[i];
        }
        out << "\n";
        

        flush(out);
        
        return 0;
    }


    if( kw.match("random", ftrigkhz) ){
        runDaqPg(0, ftrigkhz, true, 1);
        return 0;
    }
    if( kw.match("random", ftrigkhz, ntrig) ){
        runDaqPg(ntrig, ftrigkhz, true, 1);
        return 0;
    }
    
    if( kw.match("run", ftrigkhz, ntrig) ){
        runDaqPg(ntrig, ftrigkhz, false, 1);
        return 0;
    }

    int multiplier=2;
    int percentile=90;
    ntrig=1000;
    ftrigkhz = 100;
    if( kw.match("maskhot") || kw.match("maskhot",multiplier, percentile)){
        maskHotPixels(ntrig, ftrigkhz, multiplier, percentile*0.01);
        return 0;
    }
    
    ntrig=1;
    int nchunk=1;
    if( kw.match("daqtest",ntrig,nchunk) ){
      fApi->daqTriggerSource("pg_dir");
        fApi->daqStart(fDumpFlawed, 50000000, false);
        vector<rawEvent> daqRawEv;
        for(int m=0; m<nchunk; m++){
           int nerr=0;
            fApi->daqTrigger(ntrig, 500);
            try{
                daqRawEv = fApi->daqGetRawEventBuffer();
            }catch(pxar::DataNoEvent){
                out << " caught DataNoEvent exception. \n";
                break; 
            }
            
            int nword=0;
            int evtId[8]={-1,-1,-1,-1,-1,-1,-1,-1};
            if (  daqRawEv.size()==static_cast<unsigned int>(ntrig) ){
                for(unsigned int i=0; i<daqRawEv.size(); i++){
                    bool bad=false;
                    nword += daqRawEv[i].GetSize();
                    for(unsigned int j=0; j<daqRawEv[i].GetSize(); j++){
                        uint16_t w=daqRawEv.at(i)[j];
                        if ( (w&0xf000)==0xa000 ){
                            uint8_t ch =  (w&0x0700) >>8 ;
                            uint8_t id = w&0xff;
                            if ( evtId[ch]==-1 ){
                                id=evtId[ch];
                            }
                            else{
                                evtId[ch]= (evtId[ch]+1) % 256;
                                if( !(evtId[ch]==id) ){
                                    bad = true;
                                    evtId[ch]=-1;
                                }
                            }
                        }
                    }
                    if (bad  ){
                        nerr++;
                        out << dec << setw(8) << i;
                        for(unsigned int j=0; j<daqRawEv[i].GetSize(); j++){
                            out<< setw(5) << hex << daqRawEv.at(i)[j];
                        }
                        out<<endl;
                    }

                }
                out << (dec) << m << ") " << nword << " words, " << nerr << " errors" << endl;
            }else{
                out << "event number error " << daqRawEv.size() << " ntrig=" << ntrig  << "  delta=" << ntrig-daqRawEv.size() << endl;
            }
            flush(out);
        }
        fApi->daqStop(false);
        return 0;
    }
    
    
    
    if( kw.match("daqtest2",ntrig,nchunk) ){
        fApi->daqStart(fDumpFlawed, DTB_SOURCE_BUFFER_SIZE, false);
        vector<pxar::Event> events;
        for(int m=0; m<nchunk; m++){
            fApi->daqTrigger(ntrig, 500);
            try{
                events = fApi->daqGetEventBuffer();   
            }catch(pxar::DataNoEvent){
                out << " caught DataNoEvent exception. \n";
                break; 
            }
        }
        fApi->daqStop(false);
        return 0;
    }
    
    int ndac;
    if( kw.match("looptest",ntrig,ndac) ){
        fApi->_dut->testAllPixels(true);
        fApi->_dut->maskAllPixels(false);
        // get daqErrors whenever ndac*ntrig*8*4160>DTB_SOURCE_BUFFER_SIZE / 4 (tbm09)
        try{
            fApi->getEfficiencyVsDAC("CalDel", 1, ndac, fDumpFlawed |  FLAG_FORCE_MASKED, ntrig);
            uint32_t nDaqErrors= fApi->getStatistics().errors_pixel();
            out << nDaqErrors << " errors " << endl;
        }catch(pxar::DataMissingEvent){
            out << "DataMissingEvent caught\n";
        }
        return 0;
    }
    
    
    if( kw.match("daqtest3",ntrig) ){
        fApi->daqStart( fDumpFlawed, DTB_SOURCE_BUFFER_SIZE, false );
        int nevent=0;
        vector<pxar::Event> daqdat;
        uint8_t perFull;
        int totalPeriod = master->prepareDaq(100, 50); 
        int finalPeriod = fApi->daqTriggerLoop(totalPeriod);
        timer t;
        while (fApi->daqStatus(perFull) && (t.get()<30000) ) {
            if (perFull > 80) {
                out << dec << (int) perFull << "%,  pausing triggers.\n";
                flush(out);
                fApi->daqTriggerLoopHalt();
                try { daqdat = fApi->daqGetEventBuffer(); nevent+=daqdat.size(); }
                catch(pxar::DataNoEvent &) {}
                if(nevent<ntrig){
                    out << (int) nevent <<" events read. Resuming triggers.\n";
                    flush(out);
                    fApi->daqTriggerLoop(finalPeriod);
                    t=timer();
                }else{
                    break;
                }
                    
            }
        }
        if (t.get()>299999){ out << "time-out \n" ;}
        out << (int) nevent <<" events read. closings.\n";
        fApi->daqStop();
        return 0;
    }
    
    
    int trigsep, nburst,caltrig=0;
    if( kw.match("burst", ntrig, trigsep, nburst) ){ return bursttest(ntrig, trigsep, nburst);}
    if( kw.match("burst", ntrig, trigsep) ){ return bursttest(ntrig, trigsep);}
    if( kw.match("burst",ntrig) ){ return bursttest(ntrig);}
    if( kw.match("burstcal",ntrig,trigsep,caltrig) ){ return bursttest(ntrig,trigsep,1,caltrig);}
    // bursttest ntrig trigsep nburst caltrig loop

    
    if( kw.greedy_match("pgset",step, pattern, delay, comment)){
        out << "pgset " << step << " " << pattern << " " << delay;
        return 0;
    }
    
    // some api functions for playing
    if (kw.match("daqstart")){ bool stat=fApi->daqStart(); out << "stat=" << stat << "\n"; return 0;}
    if (kw.match("daqstop")){ bool stat=fApi->daqStop();out << "stat=" << stat << "\n";  return 0;}
    if (kw.match("programDUT")||kw.match("clear")){ fApi->programDUT(); return 0;}
    
    ntrig=10;
    int start=0,stop=200;
    string dacname="vthrcomp";

    if (kw.match("dacscan") || kw.match("dacscan",ntrig)
        || kw.match("dacscan",dacname,start,stop,ntrig)){
        vector<pair<uint8_t, vector<pixel> > > rresults, results;
        uint16_t FLAGS = FLAG_DISABLE_DACCAL;//0;//FLAG_FORCE_MASKED; 
        try{
            rresults = fApi->getEfficiencyVsDAC(dacname, start, stop, FLAGS, ntrig); 
        } catch(DataMissingEvent &e){
            out << "problem with readout: "<< e.what() << " missing " << e.numberMissing << " events"; 
            return 0;
       } catch(pxarException &e) {
            out << "pXar execption: "<< e.what(); 
            return 0;
        }
        copy(rresults.begin(), rresults.end(), back_inserter(results)); 
        vector<float_t> hits(16);
        for (unsigned int i = 0; i < results.size(); ++i) {
            pair<uint8_t, vector<pixel> > v = results[i];
            hits.clear();
            int idac = v.first; 
            vector<pixel> vpix = v.second;
            out << "[" << dec << setw(3) << idac << "] ";
            if(false){
                for(unsigned int k=0; k<vpix.size(); k++){
                    out << setw(6) << vpix[k].value() << " ("  << setw(2) << (int) vpix[k].roc() << ":" << (int) vpix[k].column() << "," << (int) vpix[k].row()  << ") ";
                }
            }else{
                for(unsigned int k=0; k<16; k++){ hits[k]=0;}
                for(unsigned int k=0; k<vpix.size(); k++){
                    hits[vpix[k].roc()] = vpix[k].value();
                }
                for(unsigned int k=0; k<16; k++){
                    out << setw(4) << hits[k] << " ";
                }
                
            }
            out << "\n";
        }
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
        if (kw.match(fDAC_names[i], value)){ fApi->setDAC(kw.keyword, value, rocId );  return 0 ;  }
    }
    
    for(unsigned int i=0; i<fnDAC_names; i++){
        if (kw.match(fDAC_names[i], "down", value)){ 
            uint8_t now = fApi->_dut->getDAC(rocId, kw.keyword);
            std::cout << " changing " << kw.keyword << "  from " <<(int) now << "  to " << (int) now-value <<std::endl;
            fApi->setDAC(kw.keyword, max(0,now-value), rocId);
            return 0 ; 
         }
    }
    
    if (kw.match("down", value)){ 
        uint8_t now = fApi->_dut->getDAC(rocId, "vthrcomp");
        std::cout << " changing vthrcomp from " <<(int) now << "  to " << (int) now-value <<std::endl;
        fApi->setDAC("vthrcomp", max(0,now-value), rocId);
        return 0 ; 
    }

    if (kw.match("up", value)){ 
        uint8_t now = fApi->_dut->getDAC(rocId, "vthrcomp");
        std::cout << " changing vthrcomp from " <<(int) now << "  to " << (int) now-value <<std::endl;
        fApi->setDAC("vthrcomp", min(255,now+value), rocId);
        return 0 ; 
    }

    if (kw.match("vcal",value,"hi")){
        fApi->setDAC(kw.keyword,value, rocId );
        fApi->setDAC("ctrlreg", (fApi->_dut->getDAC(rocId, "ctrlreg"))|4, rocId);
        return 0 ;
    }
    if (kw.match("vcal",value,"lo")){
        fApi->setDAC(kw.keyword,value, rocId );
        fApi->setDAC("ctrlreg", (fApi->_dut->getDAC(rocId, "ctrlreg"))&0xfb, rocId);
        return 0 ;
    }
        
    if (kw.match("hirange")) {fApi->setDAC("ctrlreg", (fApi->_dut->getDAC(rocId, "ctrlreg"))|4, rocId);return 0 ;}
    if (kw.match("lorange")) {fApi->setDAC("ctrlreg", (fApi->_dut->getDAC(rocId, "ctrlreg"))&0xfb, rocId);return 0 ;}
    if (kw.match("disable") ) {fApi->setDAC("ctrlreg", (fApi->_dut->getDAC(rocId, "ctrlreg"))|2, rocId);return 0 ;}
    if (kw.match("enable")) {fApi->setDAC("ctrlreg", (fApi->_dut->getDAC(rocId, "ctrlreg"))&0xfd, rocId);return 0 ;}
    if (kw.match("zero",value)) {fApi->setDAC("ctrlreg", (fApi->_dut->getDAC(rocId, "ctrlreg") % 0xff) | ((value&3)<<4), rocId); return 0;}
    if (kw.match("mask")   ) { fApi->_dut->maskAllPixels(true, rocId); fPixelConfigNeeded = true; return 0 ;}
    if (kw.match("cald")   ) {fApi->SetCalibrateBits(false); fApi->_dut->testAllPixels(false, rocId); fPixelConfigNeeded = true; return 0 ;}
    if (kw.match("trim")   ) {fApi->_dut->maskAllPixels(false, rocId); fPixelConfigNeeded = true; return 0 ;}
   if ( kw.match("arm",  col, 0, 51, row, 0, 79) 
      || kw.match("pixd", col, 0, 51, row, 0, 79)
      || kw.match("pixe", col, 0, 51, row, 0, 79)
      || kw.match("trim",  col, 0, 51, row, 0, 79, value) 

    ){
        // testPixel(true) : pixelConfig.enable = true => cal-inject,reported "active" by info()
        // maskPixel(true/false) : pixelConfig.mask = true /false 
        for(unsigned int c=0; c<col.size(); c++){
            for(unsigned int r=0; r<row.size(); r++){
                if(verbose) { cout << kw.keyword << " roc " << rocId << ": " << col[c] << "," << row[r] << endl; }
                if (kw.keyword=="arm"){
                    fApi->_dut->testPixel(col[c], row[r], true,  rocId); 
                    fApi->_dut->maskPixel(col[c], row[r], false, rocId);
                }else if (kw.keyword=="pixd"){
                    if(verbose){out << "masked before pixd:"<<dec << fApi->_dut->getNMaskedPixels(rocId);}
                    fApi->_dut->maskPixel(col[c], row[r], true,  rocId);
                    if(verbose){out << ", masked after pixd:"<<dec << fApi->_dut->getNMaskedPixels(rocId);}
                }else if (kw.keyword=="pixe"){
                    if(verbose){out << "masked before pixe:"<<dec << fApi->_dut->getNMaskedPixels(rocId);}
                    fApi->_dut->maskPixel(col[c], row[r], false, rocId);
                    if(verbose){out << ", masked after pixe:"<<dec << fApi->_dut->getNMaskedPixels(rocId);}
               }else if (kw.keyword=="trim"){
                   fApi->_dut->updateTrimBits(col[c], row[r], value, rocId);
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
     * core=1= TBMA, 2=TBMB, 3=both
     * return -1 for unrecognized commands
     * return  0 for success
     * return >0 for errors
     */
     if(verbose) { cout << "tbm " << kw.keyword << " ,cores =" << cores << endl;}
    int address, value, value1, value2, value3, value4, value5;
    if (kw.match("tbmset", address, value))  { return tbmset(address, value);  }
    if (kw.match("enable", "pkam")     ){ return tbmsetbit("base0",cores, 0, 0);}
    if (kw.match("disable","pkam")     ){ return tbmsetbit("base0",cores, 0, 1);}
    if (kw.match("enable", "clock")     ){ return tbmsetbit("base0",cores, 1, 0);}
    if (kw.match("disable","clock")     ){ return tbmsetbit("base0",cores, 1, 1);}
    if (kw.match("accept", "triggers") ){ return tbmsetbit("base0",cores, 4, 0);}
    if (kw.match("ignore", "triggers") ){ return tbmsetbit("base0",cores, 4, 1);}
    if (kw.match("enable", "triggers") ){ return tbmsetbit("base0",cores, 6, 0);}
    if (kw.match("disable","triggers") ){ return tbmsetbit("base0",cores, 6, 1);}
    if (kw.match("ntp")              )  { return tbmsetbit("base0",cores, 6, 1);}
    if (kw.match("enable", "autoreset")){ return tbmsetbit("base0",cores, 7, 0);}
    if (kw.match("disable","autoreset")){ return tbmsetbit("base0",cores, 7, 1);}
    if (kw.match("mode","cal"  )    ){ return tbmset("base2",cores,   0xC0);}
    if (kw.match("mode","clear")    ){ return tbmset("base2",cores,   0x80);}
    if (kw.match("mode","sync")     ){ return tbmset("base2",cores,   0x00);}
    if (kw.match("inject","trg")    ){ return tbmset("base4",cores,      1);}
    if (kw.match("hardreset")       ){ return tbmset("base4",cores, (1<<1));}
    if (kw.match("reset","roc")     ){ return tbmset("base4",cores, (1<<2));}
    if (kw.match("inject","cal")    ){ return tbmset("base4",cores, (1<<3));}
    if (kw.match("reset","tbm")     ){ return tbmset("base4",cores, (1<<4))+(1<<2);}
    if (kw.match("clear","stack")   ){ return tbmset("base4",cores, (1<<5));}
    if (kw.match("clear","token")   ){ return tbmset("base4",cores, (1<<6));}
    if (kw.match("clear","counter") ){ return tbmset("base4",cores, (1<<7));}
    if (kw.match("pkamcounter",value)){return tbmset("base8",cores,  value);}
    if (kw.match("autoreset",value)  ){return tbmset("basec",cores,  value);}
    if (kw.match("dly",value)  ){return tbmset("basea", cores, 
        (value&0x7) | ((value&0x7)<<3) , 0x38 | 0x07 );}
    if (kw.match("dly0",value)  ){return tbmset("basea", cores,  (value&0x7)   , 0x07);}
    if (kw.match("dly1",value)  ){return tbmset("basea", cores,  (value&0x7)<<3, 0x38);}
    if (kw.match("dlyhdr",value)){return tbmset("basea", cores,  (value&0x1)<<6, 0x40);}
    if (kw.match("dlytok",value)){return tbmset("basea", cores,  (value&0x1)<<7, 0x80);}
    if (kw.match("phase400",value) ){return tbmset("basee", TBMA, (value&0x7)<<2, 0x1c);}
    if (kw.match("phase160",value) ){return tbmset("basee", TBMA, (value&0x7)<<5, 0xe0);}
    if (kw.match("delay", value) ){
		if ((cores == TBMA) || (cores ==TBM1A) || (cores==TBM0A)){
			out << "delay register only in core B\n";
			return 0;
		}else{
			return tbmset("basee", cores & TBMB,  value);
		}
	}
    if (kw.match("phases") ){ return printTbmPhases();}


    if (kw.match("phases",value1, value2) ){
        return tbmset("basee", TBMA, ((value1&0x7)<<5) | ((value2&0x7)<<2), 0xfc);
    }
    if (kw.match("phases",value1, value2, value3) ){
        tbmset("basea", cores, (value3&0x7) | ((value3&0x7)<<3) , 0x38 | 0x07 );
        tbmset("basee", TBMA, ((value1&0x7)<<5) | ((value2&0x7)<<2), 0xfc);
        return 0;
    }
    if (kw.match("phases",value1, value2, value3, value4, value5) ){
        tbmset("basea", cores, ((value4&1)<<6) | ((value5&0x1)<<7) | (value3&0x7) | ((value3&0x7)<<3) );
        tbmset("basee", TBMA, ((value1&0x7)<<5) | ((value2&0x7)<<2), 0xfc);
        return 0;
    }
    int ftrigkhz=10;
    int nloop=10;
    int ntrig=100;
    if (kw.match("scan","tbm")){ return  tbmscan();}
    if (kw.match("scan","tbm",nloop)){ return tbmscan(nloop);}
    if (kw.match("scan","tbm",nloop,ntrig)){ return tbmscan(nloop,ntrig);}
    if (kw.match("scan","tbm",nloop,ntrig,ftrigkhz)){ return tbmscan(nloop,ntrig,ftrigkhz);}
    if (kw.match("scan","rocs")){ return  rocscan();}
    if (kw.match("scan","level")){ return levelscan();}
    if (kw.match("scan","raw", value)){return rawscan(value);}
    if (kw.match("timing")){return find_timing(2);}
    if (kw.match("timing2")){return find_tbmtiming(2);}
    int npass=0;
    if (kw.match("timing",npass)){return find_timing(npass);}
    if (kw.match("ports")){ return post_timing(0); }
    if (kw.match("ports",npass)){ return post_timing(npass); }
    return -1; // nothing done
}


bool CmdProc::process(Keyword keyword, Target target, bool forceTarget, int index){
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
    
    if (keyword.match("info")){
        fApi->_dut->info();
        return true;
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
    
    if( keyword.match("help") ){
        out << "Documentation can be found at ";
        out << "http://cms.web.psi.ch/phase1/software/cmdline.html\n";
        return true;
    }
    
    if( keyword.match("help","seq") ){
        out << "1=tok, 2=trg, 4=cal, 8=res, 16=sync, 32=reset-tbm\n";
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
				if(fEchoExecs) out << ">" << line << "\n"; 
                p->exec(line);  
                out << p->out.str();
            }
            delete p;
            return true;
            
        }  else {
            
            out << " Unable to open file ";;
            return false;
        }
    }
    
    if (keyword.match("verbose")){ verbose=true; return true;}
    if (keyword.match("quiet")){ verbose=false; return true;}
    if (keyword.match("target")){ out << defaultTarget.str(); return true;}
    int value;
    if (keyword.match("prerun",value)){ fPrerun=value; return true;}
    if (keyword.match("ignore","readback")){ fIgnoreReadbackErrors=true; return true;}
    if (keyword.match("verify","readback")){ fIgnoreReadbackErrors=false; return true;}
    
    if (keyword.match("timingtriggers",value)){ fNtrigTimingTest=value; return true;}
  
    if( keyword.match("dump","on") ){ fDumpFlawed=FLAG_DUMP_FLAWED_EVENTS;return true;}
    if( keyword.match("dump","off") ){ fDumpFlawed=0;  return true;}
    
    if( keyword.match("wait",value)){ pxar::mDelay( value ); return true; }
    string message;
    if ( keyword.match("echo","on")){ fEchoExecs = true; return true;}
    if ( keyword.match("echo","off")){ fEchoExecs = false; return true;}
   
    if ( keyword.match("echo","roc")){ out << "roc " << target.value() << "\n"; return true;}
    if ( keyword.match("echo","%")){ out << "%" << target.value() << "\n"; return true;}
    if ( keyword.greedy_match("echo", message) || keyword.greedy_match("log",message) ){
        out << message << "\n";
        return true;
    }
    if ( keyword.greedy_match("cout", message) ){
        std::cout << message << "\n";
        return true;
    }
    
    
    int event;
    if  ( keyword.match("decode", event)){
        vector<uint16_t> evbuf;
        int n=0;
        for(unsigned int i=0; i<fBuf.size(); i++){
            
            if(fApi->_dut->getNTbms()==0){
                if ((fBuf[i]&0x8000)==0x8000){n++;}
            }else{
                if ((fBuf[i]&0xe000)==0xa000){ n++;}
            }
            if (n==event) {evbuf.push_back( fBuf[i]); }
        
        }
        
        printData(evbuf, 1);
        return 0;
    }
    
    if  ( keyword.greedy_match("decode", message) ){
        
        vector<uint16_t>  buf;
        for(unsigned int i=0; i<keyword.narg(); i++){
            stringstream ss;
            ss << hex << keyword.argv[i].raw();
            uint16_t x=0;
            ss >> x;
            buf.push_back(x);
        }
        printData(buf, 1);
       
        return true;
    }   

    if (keyword.match("configure")){
        
        for(unsigned int core=0; core<fApi->_dut->getNTbms(); core++){
            std::vector< std::pair<std::string,uint8_t> > regs = fApi->_dut->getTbmDACs(core);
            for(unsigned int i=0; i<regs.size(); i++){
                std::cout << regs[i].first << " " << (int) regs[i].second  << std::endl;
                fApi->setTbmReg( regs[i].first, 0, core );
                fApi->setTbmReg( regs[i].first, regs[i].second, core );
            }
        }
        
        for(unsigned int rocId =0; rocId<15; rocId++){
            for(unsigned int d=0; d<fnDAC_names; d++){
                const char* dac=fDAC_names[d];
                fApi->setDAC(dac, (fApi->_dut->getDAC(rocId, dac)), rocId);
            }
        }
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
            stat =  tbm( keyword, ALLTBMS);
            if ( stat >=0 ) return (stat==0);
        }
        
        else if (target.name=="tbma")  {
            stat =  tbm( keyword, TBMA );
            if ( stat >=0 ) return (stat==0);
        }
        
        else if (target.name=="tbmb")  {
            stat =  tbm( keyword, TBMB );
            if ( stat >=0 ) return (stat==0);
        }
        
        else if (target.name=="tbm0a")  {
            stat =  tbm( keyword, TBM0A );
            if ( stat >=0 ) return (stat==0);
        }
        
        else if (target.name=="tbm0b")  {
            stat =  tbm( keyword, TBM0B );
            if ( stat >=0 ) return (stat==0);
        }
        
         else if (target.name=="tbm1a")  {
            stat =  tbm( keyword, TBM1A );
            if ( stat >=0 ) return (stat==0);
        }
        
        else if (target.name=="tbm1b")  {
            stat =  tbm( keyword, TBM1B );
            if ( stat >=0 ) return (stat==0);
        }
      
        else if (target.name=="roc")  {
            
            // special treatment of dacs: zip roc list and argument list
            for(unsigned int i=0; i<fnDAC_names; i++){
                vector<int> dacvalues;
                if(keyword.match( fDAC_names[i], dacvalues)){
                    if (dacvalues.size()>1){
                        fApi->setDAC(keyword.keyword, dacvalues[index], target.value());
                        return true;
                    }
                 }
            }
 
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


void CmdProc::stop( bool forceStop){
    fStopWhateverYouAreDoing = forceStop;
    if ( forceStop ){
        out << "stop button pressed" << endl;
    }
}

bool CmdProc::stopped( ){
    master->flush("");
    if (fStopWhateverYouAreDoing){
        fStopWhateverYouAreDoing=false;
        return true;
    }else{
        return false;
    }
}


/* driver */
int CmdProc::exec(std::string s){
    out.str("");

    //  skip empty lines and comments
    if( (s.size()==0) || (s[0]=='#') || (s[0]=='-') ) return 0;


    // pre-processing: to lower
    std::string t="";
    for(unsigned int i=0; i<s.size(); i++){
            t.push_back( tolower( s[i] ));
    }
    s=t;
    
    // parse and execute a string, leads to call-backs to CmdProc::process
    Token words( getWords(s) );
    
    words.macros=&macros;  // use the macro list of this CmdProc instance

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
        redirected = redirected | stmts[i]->redirected;
        ok |= stmts[i]->exec(this, defaultTarget);
    }

    if (ok){
        return 0;
    }else{
        return 2;
    }
}

void PixTestCmd::runCommand(std::string command) {
  std::transform(command.begin(), command.end(), command.begin(), ::tolower);
  LOG(logDEBUG) << "running command: " << command;

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

  cmd = new CmdProc( this );
  cmd->setApi(fApi, fPixSetup);

  if (!command.compare("timing")){
      std::string s_redirect = "timing > pxar_timing.log";
      cmd->exec(s_redirect.c_str());
      cmd->fApi->daqTriggerSource("pg_direct");
      cmd->fApi->daqStart(500000,true);
      cmd->fApi->daqStop(true);
  }

  PixTest::update();

}


