#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <iostream>
#include <map>
#include "sandbox.h"
#include <string>
#include <sstream>
#include <sys/types.h>
#include <vector>

using namespace std;

vector<string> files;                     // source files
vector<string> testcases;                 // testcase files
vector<string> answers;                   // answer files
vector< vector<string> > modules;         // module list
char s[8192];                             // temporary cstring variable; for input buffer or whatever else
string prefix;                            // prefix of student ID (major and admission year)

/**
 * Procedure set_quota
 * [Set quota for runtime]
 */
void set_quota(sandbox_t* sbox)
{
  sbox->task.quota[S_QUOTA_WALLCLOCK] = 1000;   // 1 sec execution limit
  sbox->task.quota[S_QUOTA_CPU] = 500;          // 0.5 sec cpu burst
  sbox->task.quota[S_QUOTA_MEMORY] = 25000;     // 25 MB memory limit
}

/**
 * Function parse
 * [Parse string towards delimiter char]
 * 
 * @param string s
 * @param char delim
 * @return vector<string>
 */
vector<string> parse(const string& s, char delim) 
{
  stringstream ss(s);                       // make stream from string
  string tmp;
  vector<string> v;

  while (getline(ss, tmp, delim))   // read all string from stream separated by delim
  {
    v.push_back(tmp);                       // push each to v
  }
  return v;
}

/**
 * Function getext
 * [Function to get extension from filename]
 * 
 * @param string& filename
 * @return string
 */
string getext(const string& filename)
{
  int pos;  
  pos = filename.find_last_of('.');   // get substring after last occurence of dot
  
  if (pos == -1) return "";
  else return filename.substr(pos, -1);
}

/**
 * Procedure getdir
 * [Append all file list to container]
 * 
 * @param string dir
 * @param vector<string>& files
 */
void getdir (const string& dir, vector<string>& files)
{
  struct dirent* dirp;
  DIR* dp;
  
  files.clear();
    
  if ((dp = opendir(dir.c_str())) == NULL)        // if directory empty pass
    return;
 
  while ((dirp = readdir(dp)) != NULL) {          // while not all files yet listed
    if (strcmp(dirp->d_name, ".")                 // if file is not . and ..
      && strcmp(dirp->d_name, ".."))
      files.push_back(string(dirp->d_name));      // append filename to container
  }
  
  closedir(dp);
}

/**
 * Function nimof
 * [Parse student ID from namefile]
 * 
 * @param string file
 * @return int
 */
int nimof (const string& file) 
{
  int pos = file.find(prefix);
  if (pos == -1) return -1;
  return stoi( file.substr( pos, 8 ) );     // convert digits after prefix to integer
}

/**
 * Function findinside
 * [Count all occurences of keyword in source file]
 * 
 * @param string file
 * @param string key
 * @return int
 */
int findinside (const string& file, const string& key)
{
  FILE* f = fopen(file.c_str(), "r"); 
  string st;
  int counter = 0, pos;
  
  while (fscanf(f, "%s", s) != EOF) {     // read all lines in file
    st = string(s);
    pos = 0;
    while ((pos = st.find(key, pos))      // while key found
      != string::npos) {
      counter++;                          // add counter
      pos++;                              // begin search after first previous character
    }
  }
  
  return counter;
}

/**
 * Procedure pushFromFile
 * [Push all line from file to vector of string]
 * 
 * @param v vector<string>&
 * @param f FILE*
 */
void pushFromFile(vector<string>& v, FILE* f)
{
  while (fgets(s, sizeof(s), f) != NULL)      // get all line from file
  {
    v.push_back(s);
  }
  fclose(f);
}

/**
 * Procedure test
 * [Test the output file with expected answer and output functionality score]
 */
void test ()
{
  FILE* f;
  vector<string> kunci;
  vector<string> jawaban;
  int skor = 0, total = 0;
  sandbox_t sbox;
  
  for (int i = 0; i < answers.size(); ++i)      // for each answer and testcase
  {
    kunci.clear();
    jawaban.clear();
    
    string ans = answers[i];
    string testcase = testcases[i];
    
    pushFromFile(kunci, fopen(("ans/" + ans).c_str(), "r"));
    
    total += kunci.size();
    
    strcpy(s, "case/");
    strcat(s, testcase.c_str());              

    const char* comm[] = {"./main", "<", s, "> output", NULL, };
    sandbox_init(&sbox, comm);
    sandbox_execute(&sbox);
    sandbox_fini(&sbox);

    if (sbox.result != S_RESULT_OK)             // if runtime error skip scoring
      continue;
    
    system("./norm");                           // normalize output

    pushFromFile(jawaban, fopen("output", "r"));
    
    for (int j = 0; j < jawaban.size() && j < kunci.size(); ++j)  // for all output
    {                                           
      skor += kunci[j] == jawaban[j];                             // check whether equal to answer
    }
  }
  
  printf("  %2d/%2d", skor, total);             // show score fraction
}

/**
 * Function headercheck
 * [Check header exist or not in source file]
 * 
 * @param string file
 * @return boolean
 */
bool headercheck (const string& file)
{
  return findinside(file, to_string(nimof(file)));
}

/**
 * Procedure modulecheck
 * [Find all module existence in source file and output module score]
 * 
 * @param string file
 */
void modulecheck (const string& file)
{
  int counter = 0;
  
  for (vector<string>& mods : modules) {
    bool exist = false;
    for (string& s : mods) {                  // if there exist an s
      exist |= (findinside(file, s) > 0);     // exist set true
    }
    counter += exist;                         // counter add if exist
  }
  
  printf("   %2d/%2d", counter, modules.size());    // print score fraction
}


/********************* MAIN PROGRAM *************************/

int main()
{
  cin >> prefix;
  
  getdir(string("."), files);                   // list all source files
  getdir(string("ans"), answers);               // list all answer files
  getdir(string("case"), testcases);            // list all testcase files
  
  FILE* f = fopen("modulelist.txt", "r");
  while (fgets(s, sizeof(s), f) != NULL) {      // list all modules
    modules.push_back(parse(string(s), '|'));
  }
  fclose(f);
  
  puts("     NIM  Compile  Score  Header  Module");
    
  for (string& file : files) 
  {
    if (file.find(".cpp") == -1) continue;      // skip unless cpp file
    
    printf("%8d", nimof(file));                 // print ID number
    
    char* com = s;
    strcpy(com, "g++ -o main ");
    strcat(com, file.c_str());
    
    // score 1 = compile
    if (system(com) == 0) {                     // if compilation return normal
      printf("  Success");                      // output success

      // score 2 = output
      test();                                   // run functionality testing
    } else {
      printf("    Error");                      // output fail
      printf("  %2d/%2d", 0, 0);                // and score set to 0 automatically
    }
    
    // score 3 = header
    // check header existence
    printf(headercheck(file) ? "   Exist" : " Lost :(");
    
    // score 4 = module
    // check program module
    modulecheck(file);
    
    puts("");
  } 
  return 0;
}

// Copyright (C) 2015 Afrizal Fikri