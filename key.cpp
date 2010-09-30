// File: key.cpp
// Written by Joshua Green

#include "shared/fileio/fileio.h"
#include "shared/db/db.h"
#include "shared/str/str.h"
#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <process.h>
#include <windows.h>
using namespace std;

const column TRANS_DATE_COL   = column("date", 30);
const column TRANS_TYPE_COL   = column("type", 30);
const column TRANS_DESC_COL   = column("description", 75);
const column TRANS_AMOUNT_COL = column("amount", 30);

const column TRANS_GRP_NAME_COL = column("name", 50);
const column TRANS_GRP_DESC_COL = column("description", 75);

const string TRANSACTION_MARKER = "<!-- end isLoc or isFdr if statement -->";
const string END_TRANSACTION_MARKER = "<!-- end for loop -->";

map<float, string> pie;

int _argc;
char** _argv;

void glbranch(void*);

struct transaction {
  string date, type, raw_description, description;
  float amount;

  row to_row() const {
    return row().add(TRANS_DATE_COL, date).add(TRANS_TYPE_COL, type).add(TRANS_DESC_COL, description).add(TRANS_AMOUNT_COL, ftos(amount));
  }
};

transaction get_transaction(fileio&);
string strip_whitespace(const string&);
string format_desc(const string&, int words=3);

int main(int argc, char** argv) {
  _argc = argc;
  _argv = argv;

  fileio data;
  database db;

  db.initialize("KeyBank");

  table* TRANSACTIONS = db.get_table("transactions");
  table* TRANSACTION_GROUPS = db.get_table("transaction_groups");

  if (TRANSACTIONS == 0) {
    cout << "Creating transaction table...";
    TRANSACTIONS = db.add_table("transactions");
    TRANSACTIONS->add_column(TRANS_DATE_COL);
    TRANSACTIONS->add_column(TRANS_TYPE_COL);
    TRANSACTIONS->add_column(TRANS_DESC_COL);
    TRANSACTIONS->add_column(TRANS_AMOUNT_COL);
    cout << " done." << endl;
  }
  else cout << "Transaction table loaded." << endl;

  if (TRANSACTION_GROUPS == 0) {
    cout << "Creating transaction-group table...";
    TRANSACTION_GROUPS = db.add_table("transaction_groups");
    TRANSACTION_GROUPS->add_column(TRANS_GRP_NAME_COL);
    TRANSACTION_GROUPS->add_column(TRANS_GRP_DESC_COL);
    cout << " done." << endl;
  }
  else cout << "Transaction-group table loaded." << endl;

  string input;
  cout << "Load new transactions? (y/n) ";
  getline(cin, input);
  if (input[0] == 'y' || input[0] == 'Y') {
    string filename;
    cout << "Filename: ";
    getline(cin, filename);

    bool define_groups = false;
    input.clear();
    cout << "Define transactions groups? (y/n) ";
    getline(cin, input);
    define_groups = (input[0] == 'y' || input[0] == 'Y');

    data.open(filename, "r");
    if (!data.is_open()) {
      cout << "Unable to open \"" << filename << "\"." << endl;
      return 1;
    }

    data.read(-1, END_TRANSACTION_MARKER);
    int stop_pos = data.pos()-40;

    data.seek(0);
    data.read(-1, TRANSACTION_MARKER);

    cout << "Loading transactions...";
    while (data.pos() < stop_pos) {
      transaction t = get_transaction(data);
      TRANSACTIONS->add_row(t.to_row());
      if (define_groups) {
        // if a group is not already defined...
        if (TRANSACTION_GROUPS->select(query().where(predicate().add(equalto(TRANS_GRP_DESC_COL, format_desc(t.description))))).size() == 0) {
          input.clear();
          cout << "\"" << t.raw_description << "\"" << endl;
          cout << "Group: " << endl;
          getline(cin, input);
          TRANSACTION_GROUPS->add_row(row().add(TRANS_GRP_NAME_COL, input).add(TRANS_GRP_DESC_COL, format_desc(t.description)));
        }
      }
    }
    cout << " done." << endl << endl;
  }

  vector<row> results = TRANSACTIONS->select(query().where());
  /*cout << "Transactions:" << endl;
  for (int i=0;i<results.size();i++) {
    results[i].print(TRANSACTIONS);
  }*/

  float total_expenses = 0.0f, total_credits = 0.0f;
  map<string, float> totals;
  for (int i=0;i<results.size();i++) {
    // search for matching group association:
    vector<row> associations = TRANSACTION_GROUPS->select(query().where(predicate().add(equalto(TRANS_GRP_DESC_COL, format_desc(results[i][TRANS_DESC_COL])))));
    if (associations.size() > 0) {
      if (totals.find(associations[0][TRANS_GRP_NAME_COL]) == totals.end()) totals[associations[0][TRANS_GRP_NAME_COL]] = 0.0f;
      totals[associations[0][TRANS_GRP_NAME_COL]] += atof(results[i][TRANS_AMOUNT_COL].c_str());
    }
    else {
      if (totals.find(format_desc(results[i][TRANS_DESC_COL])) == totals.end()) totals[format_desc(results[i][TRANS_DESC_COL])] = 0.0f;
      totals[format_desc(results[i][TRANS_DESC_COL])] += atof(results[i][TRANS_AMOUNT_COL].c_str());
    }

    if (atof(results[i][TRANS_AMOUNT_COL].c_str()) < 0) total_expenses += atof(results[i][TRANS_AMOUNT_COL].c_str());
    else total_credits += atof(results[i][TRANS_AMOUNT_COL].c_str());
  }

  map<string, float>::iterator it = totals.begin();
  while (it != totals.end()) {
    for (int i=0;i<30;i++) {
      if (i < it->first.length()) cout << (it->first)[i];
      else cout << " ";
    }
    cout << ": ";
    if (it->second >= 0) cout << " ";
    cout << it->second << "\t(";

    if (it->second >= 0)  cout << floor((it->second/total_credits)*1000.0f + 0.5f)/10.0f << "%)" << endl;
    else {
                          cout << floor((it->second/total_expenses)*1000.0f + 0.5f)/10.0f << "%)" << endl;
      pie[fabs(floor((it->second/total_expenses)*1000.0f + 0.5f)/10.0f)/100.0f] = it->first;
    }

    it++;
  }

  // ---------------------- OPENGL ---------------------- //
  _beginthread(&glbranch, 0, (void*)0);
  // -------------------- END OPENGL -------------------- //

  input.clear();
  cout << "Expand group? (y/n) ";
  getline(cin, input);
  if (input[0] == 'y' || input[0] == 'Y') {
    cout << "Group: ";
    input.clear();
    getline(cin, input);
    vector<row> groups = TRANSACTION_GROUPS->select(query().where(predicate().And(equalto(TRANS_GRP_NAME_COL, input))));
    for (int groups_index=0;groups_index< groups.size();groups_index++) {
      string group_description = groups[groups_index][TRANS_GRP_DESC_COL];
      vector<row> transactions = TRANSACTIONS->select(query().where(predicate().And(equalto(TRANS_DESC_COL, group_description))));

      for (int c=0;c<transactions.size();c++) {
        cout << "  ";
        string transaction_description = transactions[c][TRANS_DESC_COL];
        float   transaction_amount = atof(transactions[c][TRANS_AMOUNT_COL].c_str());

        for (int i=0;i<30;i++) {
          if (i < transaction_description.length()) cout << transaction_description[i];
          else cout << " ";
        }

        cout << ": ";
        if (transaction_amount >= 0) cout << " ";
        cout << transaction_amount << "\t(";

        if (transaction_amount >= 0)  cout << floor((transaction_amount/total_credits)*1000.0f + 0.5f)/10.0f << "%)" << endl;
        else                  cout << floor((transaction_amount/total_expenses)*1000.0f + 0.5f)/10.0f << "%)" << endl;

      }
    }
  }

  return 0;
}

transaction get_transaction(fileio& data) {
  transaction entry;
  string temp;

  data.read(-1, " >");
  entry.date = strip_whitespace(data.read(-1, "</td>"));
  data.read(-1, " >");
  entry.type = strip_whitespace(data.read(-1, "</td>"));
  data.read(-1, " >");
  entry.raw_description = data.read(-1, "</td>");
  entry.description = format_desc(entry.raw_description);

  data.read(-1, "align=\"right\">");
  temp = data.read(-1, "</td>");
  if (temp != "&nbsp;") {
    temp.insert(0, string("-"));
    entry.amount = atof(temp.c_str());
  }
  else {
    data.read(-1, "align=\"right\">");
    temp = data.read(-1, "</td>");
    entry.amount = atof(temp.c_str());
  }

  data.read(-1, "</tr>");

  return entry;
}

string strip_whitespace(const string& t) {
  string str = t;

  int del = 0;
  for (int i=0;i<str.length();i++) {
    if ( (str[i] == ' ' || str[i] == '\n' || str[i] == '\t' || str[i] == '\r')
        && (i+1 < str.length() && (str[i+1] == ' ' || str[i+1] == '\n' || str[i+1] == '\t' || str[i+1] == '\r'))
       ) del++;
    else {
      str.erase(i-del, del);
      i-= del;
      del = 0;
    }
  }
  
  for (int i=str.length()-1;i>0;i--) {
    if (str[i] == ' ' || str[i] == '\t' || str[i] == '\n' || str[i] == '\r') str.erase(i, 1);
    else break;
  }

  return str;
}

string format_desc(const string& t, int words) {
  string str;

  int i=0;
  while (i < t.length()) {
    if (t[i] == ' ') words--;
    if (words > 0) {
      // is alpha or numeric
      if (t[i] == ' ' || (t[i] > 64 && t[i] < 91) || (t[i] > 96 && t[i] < 123)) str += t[i];
    }
    else break;
    i++;
  }

  return strtolower(strip_whitespace(str));
}
